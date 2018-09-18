/* Copyright (C) 2009-2010,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>

#pragma GCC diagnostic push  // require GCC 4.6
#pragma GCC diagnostic ignored "-Wformat"

struct thrctrl
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_cond_t safepoint_cond;      // "paused" threads will wait on this condition
    pthread_mutex_t safepoint_lock;

    int running_threads;
    int booted;
    int gc_needed;
    int requested_by;
} TC;

#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)

#define TC_LOCK if (DEBUG) fprintf(stderr, "%d] TC_LOCK thr:%d boot:%d\n", PTHREAD_NUM, TC.running_threads, TC.booted); IFED(pthread_mutex_lock(&TC.lock))
#define TC_UNLOCK if (DEBUG) fprintf(stderr, "%d] TC_UNLOCK thr:%d boot:%d\n", PTHREAD_NUM, TC.running_threads, TC.booted); IFED(pthread_mutex_unlock(&TC.lock))
#define TCSP_LOCK if (DEBUG) fprintf(stderr, "%d] TCSP_LOCK thr:%d boot:%d\n", PTHREAD_NUM, TC.running_threads, TC.booted); IFED(pthread_mutex_lock(&TC.safepoint_lock))
#define TCSP_UNLOCK if (DEBUG) fprintf(stderr, "%d] TCSP_UNLOCK thr:%d boot:%d\n", PTHREAD_NUM, TC.running_threads, TC.booted); IFED(pthread_mutex_unlock(&TC.safepoint_lock))

/*
 * - threads can ask for GC's by setting gc_needed to 1
 *   and requested_by to their threadid. in the current
 *   model, only the most recent requester (eg the one 
 *   right before running_threads hits zero) is noted.
 * - threads can enter safe point (for a GC) by decrementing
 *   running_threads by one.
 * - A thread may leave a safe point only if gc_needed is zero.
 * - Upon leaving the safe point, running_threads is incremented by one.
 *
 * ask_for_gc:
 *    lock(gc_needed = 1)
 *
 * enter_safe_point:
 *    lock(running_threads --)
 *
 * leave_safe_point:
 *    spin while (gc_needed == 1)
 *    lock(running_threads ++)
 *
 */

#define pthread_yield sched_yield

#define REQUESTGC do { \
        if (DEBUG) fprintf(stderr, "%d] REQUESTGC start thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        TC_LOCK; TC.gc_needed = 1; TC.requested_by = PTHREAD_NUM; setup_for_gc(s); TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] REQUESTGC end thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define COMPLETEGC do { \
        if (DEBUG) fprintf(stderr, "%d] COMPLETEGC start thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        finish_for_gc(s); \
        if (DEBUG) fprintf(stderr, "%d] COMPLETEGC end thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define ENTER_SAFEPOINT do { \
        if (DEBUG) fprintf(stderr, "%d] ENTER_SAFEPOINT start thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        TC_LOCK; TC.running_threads--; pthread_cond_signal(&TC.cond); TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] ENTER_SAFEPOINT end thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define LEAVE_SAFEPOINT do { \
        if (DEBUG) fprintf(stderr, "%d] LEAVE_SAFEPOINT start thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        TCSP_LOCK; while (TC.gc_needed) pthread_cond_wait(&TC.safepoint_cond, &TC.safepoint_lock); TCSP_UNLOCK; \
        TC_LOCK; TC.running_threads++; TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] LEAVE_SAFEPOINT end thr:%d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)


#ifndef CHECKDISABLEGC
#define CHECKDISABLEGC do { if (getenv("DISABLEGC")) { fprintf(stderr, "GC is disabled\n"); return; } } while(0)
#endif
void minorGC (GC_state s) {
    //minorCheneyCopyGC (s);
}


void
growStackCurrent (GC_state s)
{
    size_t reserved;
    GC_stack stack;

    reserved = sizeofStackGrowReserved (s, getStackCurrent (s));
    if (DEBUG_STACKS or s->controls.messages)
        fprintf (stderr,
                 "[GC: Growing stack of size %s bytes to size %s bytes, using %s bytes.]\n",
                 uintmaxToCommaString (getStackCurrent (s)->reserved),
                 uintmaxToCommaString (reserved),
                 uintmaxToCommaString (getStackCurrent (s)->used));

    /* TODO insufficient heap will cause grow to fail since we've now separated
     * stack ops from heap ops
     */

    if (not hasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0)) {
        if (DEBUG_STACKS or s->controls.messages)
            fprintf (stderr,
                     "%d]No heap bytes free to grow stack hence calling GC\n",
                     PTHREAD_NUM);
        //             resizeHeap (s, s->lastMajorStatistics.bytesLive + sizeofStackWithHeader(s,reserved));
    }


//  assert (hasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0));
    stack = newStack (s, reserved, TRUE);
    copyStack (s, getStackCurrent (s), stack);
    getThreadCurrent (s)->stack =
        pointerToObjptr ((pointer) stack, s->heap.start);

    s->stackBottom[PTHREAD_NUM] = getStackBottom (s, stack);
    s->stackTop[PTHREAD_NUM] = getStackTop (s, stack);
    s->stackLimit[PTHREAD_NUM] = getStackLimit (s, stack);
}

void
maybe_growstack (GC_state s)
{
    bool stackTopOk;
    if (isStackEmpty (getStackCurrent (s)))
        return;
    stackTopOk = invariantForMutatorStack (s);
#if 0
    int stackBytesRequested =
        stackTopOk
        ? 0
        : sizeofStackWithHeader (s,
                                 sizeofStackGrowReserved (s,
                                                          getStackCurrent
                                                       (s)));
#endif
    unless (stackTopOk) growStackCurrent (s);
}

void
enterGC (GC_state s)
{
    if (s->profiling.isOn) {
        /* We don't need to profileEnter for count profiling because it
         * has already bumped the counter.  If we did allow the bump, then
         * the count would look like function(s) had run an extra time.
         */
        if (s->profiling.stack and not (PROFILE_COUNT == s->profiling.kind))
            GC_profileEnter (s);
    }
    s->amInGC = TRUE;
}

/* TODO
 * define THREADED to enable threading (right now its linear just like
 * the original code)
 */
void
leaveGC (GC_state s)
{
    if (s->profiling.isOn) {
        if (s->profiling.stack and not (PROFILE_COUNT == s->profiling.kind))
            GC_profileLeave (s);
    }
    s->amInGC = FALSE;
}

#define THREADED

#define MYASSERT(T, X, COMP, RV) {                               \
	  T __rv__ = (T)X;                                           \
        if (!(__rv__ COMP (T)RV)) {                              \
                fprintf(stderr, #X " failed, %x " #COMP " %x\n", \
                                __rv__, (T)RV);                  \
                exit(-1);                                        \
        }                                                        \
}

#define COPYIN(EL) s->EL[1] = s->EL[TC.requested_by]
#define COPYOUT(EL) s->EL[TC.requested_by] = s->EL[1]
#define SANITY(EL) if (s->EL[TC.requested_by] == s->EL[1]) if (DEBUG) fprintf(stderr,"%d] " #EL " changed!\n", PTHREAD_NUM);

static void
setup_for_gc (GC_state s)
{
    COPYIN (stackTop);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCREqBy = %d , before copy stackBottom = " FMTPTR
                 " , should become = " FMTPTR " , actually = " FMTPTR
                 " \n", PTHREAD_NUM, TC.requested_by, s->stackBottom[1],
                 s->stackBottom[TC.requested_by], s->stackBottom[0]);
    COPYIN (stackBottom);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCReqBy= %d,  after copy StackBottom = " FMTPTR " \n",
                 PTHREAD_NUM, TC.requested_by, s->stackBottom[1]);
    COPYIN (stackLimit);
    COPYIN (exnStack);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCREqBy = %d , before copy currentThread = "FMTPTR" , should become = "FMTPTR" , main thread = "FMTPTR" \n", PTHREAD_NUM, TC.requested_by,
                 s->currentThread[1],s->currentThread[TC.requested_by],s->currentThread[0]);
    COPYIN (currentThread);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCReqBy= %d,  after copy currentThread = "FMTPTR" \n", PTHREAD_NUM, TC.requested_by,s->currentThread[1]);
    COPYIN (savedThread);
    COPYIN (signalHandlerThread);
    COPYIN (ffiOpArgsResPtr);
}

static void
sanity_check_array (GC_state s)
{
    SANITY (stackTop);
    SANITY (stackBottom);
    SANITY (stackLimit);
    SANITY (exnStack);
    SANITY (currentThread);
    SANITY (savedThread);
    SANITY (signalHandlerThread);
    SANITY (ffiOpArgsResPtr);
}

static void
finish_for_gc (GC_state s)
{
    sanity_check_array (s);
    COPYOUT (stackTop);
    COPYOUT (stackBottom);
    COPYOUT (stackLimit);
    COPYOUT (exnStack);
    COPYOUT (currentThread);
    COPYOUT (savedThread);
    COPYOUT (signalHandlerThread);
    COPYOUT (ffiOpArgsResPtr);
}



__attribute__ ((noreturn))
     void *GCrunner (void *_s)
{
    GC_state s = (GC_state) _s;

    set_pthread_num (1);        // by definition
    TC.running_threads = 1;     // main thr
    TC.gc_needed = 0;
    TC.booted = 0;
    TC.requested_by = 0;

    pthread_mutex_init (&TC.lock, NULL);
    pthread_mutex_init (&TC.safepoint_lock, NULL);
    pthread_cond_init (&TC.cond, NULL);
    pthread_cond_init (&TC.safepoint_cond, NULL);

    if (DEBUG)
        fprintf (stderr, "%d] GC_Runner Thread running.\n", PTHREAD_NUM);

    s->GCrunnerRunning = TRUE;

#ifdef THREADED
    while (1) {
        if (DEBUG)
            fprintf (stderr, "%d] GCrunner: waiting for GC request.\n",
                     PTHREAD_NUM);

        TC_LOCK;
        do {
            do {
                if (DEBUG)
                    fprintf (stderr,
                             "%d] TC_UNLOCK (implied) thr:%d boot:%d\n",
                             PTHREAD_NUM, TC.running_threads, TC.booted);
                pthread_cond_wait (&TC.cond, &TC.lock); // implicit TC_UNLOCK
            }
            while (TC.booted && TC.running_threads);
        }
        while (!TC.gc_needed);

        // TC_LOCK is re-acquired here as a result of cond_wait succeeding

        if (DEBUG)
            fprintf (stderr,
                     "%d] GCrunner: GC requested. all threads should be paused.\n",
                     PTHREAD_NUM);

        // at this point, all threads should be paused and the GC can proceed    

        if (s->isRealTimeThreadInitialized) {
            if (DEBUG) {
                fprintf (stderr,
                         "%d] GCrunner: threads paused. GC'ing\n",
                         PTHREAD_NUM);
                fprintf (stderr,
                         "%d] GC running needed=%d threads=%d\n",
                         PTHREAD_NUM, TC.gc_needed, TC.running_threads);
            }

            performUMGC(s, 3000, 0, true);

            if (DEBUG)
                fprintf (stderr,
                         "%d] GCrunner: finished. unpausing threads.\n",
                         PTHREAD_NUM);

            TC.gc_needed = 0;
            pthread_cond_broadcast (&TC.safepoint_cond);        // unpause all threads
            TC_UNLOCK;
        }
        else {
            fprintf (stderr,
                     "%d] GCrunner: skipping thread pause bc RTT is not yet initialized\n",
                     PTHREAD_NUM);
        }
    }
#endif

 /*NOTREACHED*/}



void performUMGC(GC_state s,
                 size_t ensureObjectChunksAvailable,
                 size_t ensureArrayChunksAvailable,
                 bool fullGC) {

    if (DEBUG_MEM) {
        fprintf(stderr, "PerformUMGC\n");
        dumpUMHeap(s);
    }


#ifdef PROFILE_UMGC
    long t_start = getCurrentTime();
    fprintf(stderr, "[GC] Free chunk: %d, Free array chunk: %d\n",
            s->fl_chunks,
            s->fl_chunks);
#endif

    GC_stack currentStack = getStackCurrent(s);
    foreachGlobalObjptr (s, umDfsMarkObjectsMark);
    foreachObjptrInObject(s, (pointer) currentStack, umDfsMarkObjectsMark, FALSE);

//    foreachGlobalObjptr (s, dfsMarkWithoutHashConsWithLinkWeaks);
//    GC_stack currentStack = getStackCurrent(s);
//    foreachObjptrInObject(s, currentStack,
//                          dfsMarkWithoutHashConsWithLinkWeaks, FALSE);



    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk)+sizeof(Word32_t); /*account for 4 bytes of chunktype header*/
    pointer end = s->umheap.start + s->umheap.size - step;


    for (pchunk=s->umheap.start;
         pchunk < end;
         pchunk+=step) {
        if(((UM_Mem_Chunk)pchunk)->chunkType == UM_NORMAL_CHUNK)
        {
        GC_UM_Chunk pc = (GC_UM_Chunk)(pchunk+4); /*account for size of chunktype*/
        if ((pc->chunk_header & UM_CHUNK_IN_USE) &&
            (!(pc->chunk_header & UM_CHUNK_HEADER_MASK))) {
            if (DEBUG_MEM) {
                fprintf(stderr, "Collecting: "FMTPTR", %d, %d\n",
                        (uintptr_t)pc, pc->sentinel, pc->chunk_header);
            }
            insertFreeChunk(s, &(s->umheap), pchunk);
        }

        }
        else if(((UM_Mem_Chunk)pchunk)->chunkType == UM_ARRAY_CHUNK)
        {

        GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)(pchunk + 4); /*account for size of chunktype*/
        if ((pc->array_chunk_header & UM_CHUNK_IN_USE) &&
            (!(pc->array_chunk_header & UM_CHUNK_HEADER_MASK))) {
            if (DEBUG_MEM) {
                fprintf(stderr, "Collecting array: "FMTPTR", %d, %d\n",
                        (uintptr_t)pc, pc->array_chunk_magic,
                        pc->array_chunk_header);
            }
            insertFreeChunk(s, &(s->umheap), pchunk);
        }
        }

        if (!fullGC && s->fl_chunks >= ensureObjectChunksAvailable) {
            break;
        }
    }

    foreachObjptrInObject(s, (pointer) currentStack, umDfsMarkObjectsUnMark, FALSE);
    foreachGlobalObjptr (s, umDfsMarkObjectsUnMark);

#ifdef PROFILE_UMGC
    long t_end = getCurrentTime();
    fprintf(stderr, "[GC] Time: %ld, Free chunk: %d, Free array chunk: %d, "
            "ensureArrayChunk: %d\n",
            t_end - t_start,
            s->fl_chunks,
            s->fl_chunks,
            ensureArrayChunksAvailable);
#endif

}


void GC_collect (GC_state s, size_t bytesRequested, bool force) {
    if (!force) {
        if ((s->fl_chunks > 2000))// &&
            //(s->fl_array_chunks > 1000000))
            return;
    }

    if (s->gc_module == GC_NONE) {
        return;
    }

    /*GC_collect_real(s, bytesRequested, true);*/

}


#pragma GCC diagnostic pop  // require GCC 4.6
