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
    minorCheneyCopyGC (s);
}



 __attribute__ ((unused)) void majorGC (GC_state s, size_t bytesRequested, bool mayResize)
{
    uintmax_t numGCs;
    size_t desiredSize;
    CHECKDISABLEGC;
    
    if(DEBUG)
    fprintf (stderr, "%d] [GC: Starting Major GC...]\n", PTHREAD_NUM);

    s->lastMajorStatistics.numMinorGCs = 0;
    numGCs =
        s->cumulativeStatistics.numCopyingGCs
        + s->cumulativeStatistics.numMarkCompactGCs;
    if (0 < numGCs
        and ((float) (s->cumulativeStatistics.numHashConsGCs) /
             (float) (numGCs) < s->controls.ratios.hashCons))
        s->hashConsDuringGC = TRUE;
    desiredSize =
        sizeofHeapDesired (s,
                           s->lastMajorStatistics.bytesLive + bytesRequested,
                           0);
    if (not FORCE_MARK_COMPACT and not s->hashConsDuringGC      // only markCompact can hash cons
        and s->heap.withMapsSize < s->sysvals.ram
        and (not isHeapInit (&s->secondaryHeap)
             or createHeapSecondary (s, desiredSize)))
        majorCheneyCopyGC (s);
    else
        majorMarkCompactGC (s);
    s->hashConsDuringGC = FALSE;
    s->lastMajorStatistics.bytesLive = s->heap.oldGenSize;
    if (s->lastMajorStatistics.bytesLive >
        s->cumulativeStatistics.maxBytesLive)
        s->cumulativeStatistics.maxBytesLive =
            s->lastMajorStatistics.bytesLive;
    /* Notice that the s->lastMajorStatistics.bytesLive below is
     * different than the s->lastMajorStatistics.bytesLive used as an
     * argument to createHeapSecondary above.  Above, it was an
     * estimate.  Here, it is exactly how much was live after the GC.
     */
    if (mayResize) {
        resizeHeap (s, s->lastMajorStatistics.bytesLive + bytesRequested);
    }
    setCardMapAndCrossMap (s);
    resizeHeapSecondary (s);
    assert (s->heap.oldGenSize + bytesRequested <= s->heap.size);
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
        ensureHasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0);
    }


//  assert (hasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0));
    stack = newStack (s, reserved, TRUE);
    copyStack (s, getStackCurrent (s), stack);
    getThreadCurrent (s)->stack =
        pointerToObjptr ((pointer) stack, s->heap.start);
    markCard (s, objptrToPointer (getThreadCurrentObjptr (s), s->heap.start));

    s->stackBottom[PTHREAD_NUM] = getStackBottom (s, stack);
    s->stackTop[PTHREAD_NUM] = getStackTop (s, stack);
    s->stackLimit[PTHREAD_NUM] = getStackLimit (s, stack);
}

void
maybe_growstack (GC_state s)
{
    bool stackTopOk;
    size_t stackBytesRequested;
    if (isStackEmpty (getStackCurrent (s)))
        return;
    stackTopOk = invariantForMutatorStack (s);
    stackBytesRequested =
        stackTopOk
        ? 0
        : sizeofStackWithHeader (s,
                                 sizeofStackGrowReserved (s,
                                                          getStackCurrent
                                                          (s)));
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

#undef GCTHRDEBUG

#ifdef GCTHRDEBUG
#define DBG(X) fprintf X
#else
#define DBG(X)
#endif

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
                 "%d] GCREqBy = %d , before copy stackBottom = %" PRIuMAX
                 " , should become = %" PRIuMAX " , actually = %" PRIuMAX
                 " \n", PTHREAD_NUM, TC.requested_by, s->stackBottom[1],
                 s->stackBottom[TC.requested_by], s->stackBottom[0]);
    COPYIN (stackBottom);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCReqBy= %d,  after copy StackBottom = %" PRIuMAX " \n",
                 PTHREAD_NUM, TC.requested_by, s->stackBottom[1]);
    COPYIN (stackLimit);
    COPYIN (exnStack);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCREqBy = %d , before copy currentThread = %x , should become = %x , main thread = %x \n", PTHREAD_NUM, TC.requested_by,
                 s->currentThread[1],s->currentThread[TC.requested_by],s->currentThread[0]);
    COPYIN (currentThread);
    if (DEBUG)
        fprintf (stderr,
                 "%d] GCReqBy= %d,  after copy currentThread = %x \n", PTHREAD_NUM, TC.requested_by,s->currentThread[1]);
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

            performGC_helper (s,
                              s->oldGenBytesRequested,
                              s->nurseryBytesRequested,
                              s->forceMajor, s->mayResize);

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

    pthread_exit (NULL);
 /*NOTREACHED*/}

void
performGC (GC_state s,
           size_t oldGenBytesRequested,
           size_t nurseryBytesRequested, bool forceMajor, bool mayResize)
{
    if (DEBUG)
        fprintf (stderr, "%d] performGC: starting..\n", PTHREAD_NUM);

    CHECKDISABLEGC;

    /* In our MT formulation of realtime MLton, we move the
     * GC into a separate pthread. In order to keep things
     * orderly, we implement a STW approach to GC. The GC
     * waits on mutex acquisition. When it acquires it, that
     * indicates a GC was requested by one of the other threads.
     * Before progressing, the GC must ask all of the threads to
     * pause. We do this by using a condition variable that the 
     * threads wait on when they are at a safe point.
     * 
     * Once all threads are paused on the condition, the GC can 
     * proceed. Once the GC finishes, it signals all of the 
     * threads waiting on the condition, causing them to wakeup.
     */

#ifdef THREADED
    s->oldGenBytesRequested = oldGenBytesRequested;
    s->nurseryBytesRequested = nurseryBytesRequested;
    s->forceMajor = forceMajor;
    s->mayResize = mayResize;

    while (s->isRealTimeThreadInitialized == FALSE) {
        if (DEBUG) {
            fprintf (stderr, "%d] spin [can't performGC yet] ..\n",
                     PTHREAD_NUM);
        }
        ssleep (1, 0);
    }

    REQUESTGC;
    ENTER_SAFEPOINT;
    sched_yield ();
    LEAVE_SAFEPOINT;
    COMPLETEGC;

#else
    DBG ((stderr, "non-threaded mode, passing thru to performGC_helper\n"));
    performGC_helper (s, oldGenBytesRequested, nurseryBytesRequested,
                      forceMajor, mayResize);
#endif
}



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
    size_t step = sizeof(struct GC_UM_Chunk);
    pointer end = s->umheap.start + s->umheap.size - step;
#if 0
    //    if (s->umheap.fl_chunks <= 2000) {
    for (pchunk=s->umheap.start;
         pchunk < end;
         pchunk+=step) {
        GC_UM_Chunk pc = (GC_UM_Chunk)pchunk;
        if ((pc->chunk_header & UM_CHUNK_IN_USE) &&
            (!(pc->chunk_header & UM_CHUNK_HEADER_MASK))) {
            if (DEBUG_MEM) {
                fprintf(stderr, "Collecting: "FMTPTR", %d, %d\n",
                        (uintptr_t)pc, pc->sentinel, pc->chunk_header);
            }
            insertFreeChunk(s, &(s->umheap), pchunk);
        }

        if (!fullGC && s->fl_chunks >= ensureObjectChunksAvailable) {
            break;
        }
    }
        //    }

    step = sizeof(struct GC_UM_Array_Chunk);
    end = s->umarheap.start + s->umarheap.size - step;

    for (pchunk=s->umarheap.start;
         pchunk < end;
         pchunk += step) {
        GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)pchunk;
        if ((pc->array_chunk_header & UM_CHUNK_IN_USE) &&
            (!(pc->array_chunk_header & UM_CHUNK_HEADER_MASK))) {
            if (DEBUG_MEM) {
                fprintf(stderr, "Collecting array: "FMTPTR", %d, %d\n",
                        (uintptr_t)pc, pc->array_chunk_magic,
                        pc->array_chunk_header);
            }
            insertFreeChunk(s, &(s->umarheap), pchunk);
        }

        /* if (!fullGC && */
        /*     s->fl_chunks >= ensureArrayChunksAvailable) { */
        /*     fprintf(stderr, "Array chunk ensured\n"); */
        /*     break; */
        /* } */
    }

#endif

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

void performGC_helper (GC_state s,
                size_t oldGenBytesRequested,
                size_t nurseryBytesRequested,
                bool forceMajor,
                __attribute__ ((unused)) bool mayResize) {
  uintmax_t gcTime;
  bool stackTopOk;
  size_t stackBytesRequested;
  struct rusage ru_start;
  size_t totalBytesRequested;
//  if (s->gc_module == GC_UM) {
//      performUMGC(s);
//	  return;
//  }


  if (s->gc_module == GC_NONE) {
      return;
  }

  enterGC (s);
  s->cumulativeStatistics.numGCs++;
  if (DEBUG or s->controls.messages) {
    size_t nurserySize = s->heap.size - ((size_t)(s->heap.nursery - s->heap.start));
    size_t nurseryUsed = (size_t)(s->frontier - s->heap.nursery);
    fprintf (stderr,
             "[GC: Starting gc #%s; requesting %s nursery bytes and %s old-gen bytes,]\n",
             uintmaxToCommaString(s->cumulativeStatistics.numGCs),
             uintmaxToCommaString(nurseryBytesRequested),
             uintmaxToCommaString(oldGenBytesRequested));
    fprintf (stderr,
             "[GC:\theap at "FMTPTR" of size %s bytes (+ %s bytes card/cross map),]\n",
             (uintptr_t)(s->heap.start),
             uintmaxToCommaString(s->heap.size),
             uintmaxToCommaString(s->heap.withMapsSize - s->heap.size));
    fprintf (stderr,
             "[GC:\twith old-gen of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(s->heap.oldGenSize),
             100.0 * ((double)(s->heap.oldGenSize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(nurserySize),
             100.0 * ((double)(nurserySize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery using %s bytes (%.1f%% of heap, %.1f%% of nursery).]\n",
             uintmaxToCommaString(nurseryUsed),
             100.0 * ((double)(nurseryUsed) / (double)(s->heap.size)),
             100.0 * ((double)(nurseryUsed) / (double)(nurserySize)));
  }
  assert (invariantForGC (s));
  if (needGCTime (s))
    startTiming (&ru_start);
//  minorGC (s);
  stackTopOk = invariantForMutatorStack (s);
#if 0
  stackBytesRequested =
    stackTopOk
    ? 0
    : sizeofStackWithHeader (s, sizeofStackGrowReserved (s, getStackCurrent (s)));
#endif
  totalBytesRequested =
    oldGenBytesRequested
    + nurseryBytesRequested
    + stackBytesRequested;
  if (forceMajor
      or totalBytesRequested > s->heap.size - s->heap.oldGenSize) {
//    majorGC (s, totalBytesRequested, mayResize);
      performUMGC(s, 3000, 0, true);
  }

  setGCStateCurrentHeap (s, oldGenBytesRequested + stackBytesRequested,
                         nurseryBytesRequested);
  assert (hasHeapBytesFree (s, oldGenBytesRequested + stackBytesRequested,
                            nurseryBytesRequested));
#if 0
  unless (stackTopOk)
    growStackCurrent (s);
#endif
  setGCStateCurrentThreadAndStack (s);
  if (needGCTime (s)) {
    gcTime = stopTiming (&ru_start, &s->cumulativeStatistics.ru_gc);
    s->cumulativeStatistics.maxPauseTime =
      max (s->cumulativeStatistics.maxPauseTime, gcTime);
  } else
    gcTime = 0;  /* Assign gcTime to quell gcc warning. */
  if (DEBUG or s->controls.messages) {
    size_t nurserySize = s->heap.size - (size_t)(s->heap.nursery - s->heap.start);
    fprintf (stderr,
             "[GC: Finished gc #%s; time %s ms,]\n",
             uintmaxToCommaString(s->cumulativeStatistics.numGCs),
             uintmaxToCommaString(gcTime));
    fprintf (stderr,
             "[GC:\theap at "FMTPTR" of size %s bytes (+ %s bytes card/cross map),]\n",
             (uintptr_t)(s->heap.start),
             uintmaxToCommaString(s->heap.size),
             uintmaxToCommaString(s->heap.withMapsSize - s->heap.size));
    fprintf (stderr,
             "[GC:\twith old-gen of size %s bytes (%.1f%% of heap),]\n",
             uintmaxToCommaString(s->heap.oldGenSize),
             100.0 * ((double)(s->heap.oldGenSize) / (double)(s->heap.size)));
    fprintf (stderr,
             "[GC:\tand nursery of size %s bytes (%.1f%% of heap).]\n",
             uintmaxToCommaString(nurserySize),
             100.0 * ((double)(nurserySize) / (double)(s->heap.size)));
  }
  /* Send a GC signal. */
  if (s->signalsInfo.gcSignalHandled
      and s->signalHandlerThread != BOGUS_OBJPTR) {
    if (DEBUG_SIGNALS)
      fprintf (stderr, "GC Signal pending.\n");
    s->signalsInfo.gcSignalPending = TRUE;
    unless (s->signalsInfo.amInSignalHandler)
      s->signalsInfo.signalIsPending = TRUE;
  }
  if (DEBUG)
    displayGCState (s, stderr);
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
  assert (invariantForGC (s));
  leaveGC (s);
}


void ensureInvariantForMutator (GC_state s, bool force) {
    force = true;
    performGC (s, 0, getThreadCurrent(s)->bytesNeeded, force, TRUE);

    assert (invariantForMutatorFrontier(s));
    assert (invariantForMutatorStack(s));
/*
void
ensureInvariantForMutator (GC_state s, bool force)
{
    if (DEBUG)
        fprintf (stderr, "%d] ensureInvariantForMutator\n", PTHREAD_NUM);

    if (force or not (invariantForMutatorFrontier (s))) {
        performGC (s, 0, getThreadCurrent (s)->bytesNeeded, force, TRUE);
    }

    if (not (invariantForMutatorStack (s)))
        maybe_growstack (s);

    assert (invariantForMutatorFrontier (s));
    if (DEBUG)
        fprintf (stderr, "%d] ensureInvariantForMutatorStack 2nd call\n",
                 PTHREAD_NUM);
    assert (invariantForMutatorStack (s));
>>>>>>> develop/rt-threading
*/

}

/* ensureHasHeapBytesFree (s, oldGen, nursery)
 */

void GC_collect_real(GC_state s, size_t bytesRequested, bool force) {
  enter (s);
  /* When the mutator requests zero bytes, it may actually need as
   * much as GC_HEAP_LIMIT_SLOP.
   */
  if (0 == bytesRequested)
    bytesRequested = GC_HEAP_LIMIT_SLOP;
  getThreadCurrent(s)->bytesNeeded = bytesRequested;
  switchToSignalHandlerThreadIfNonAtomicAndSignalPending (s);
  ensureInvariantForMutator (s, force);
  assert (invariantForMutatorFrontier(s));
  assert (invariantForMutatorStack(s));
  leave (s);

  if (DEBUG_MEM) {
      fprintf(stderr, "GC_collect done\n");
  }
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

    GC_collect_real(s, bytesRequested, true);

}

void ensureHasHeapBytesFree (GC_state s,
                        size_t oldGenBytesRequested,
                        size_t nurseryBytesRequested)
{
    assert (s->heap.nursery <= s->limitPlusSlop);
    assert (s->frontier <= s->limitPlusSlop);

    if (DEBUG) {
        displayHeap (s, &(s->heap), stderr);
        displayHeapInfo (s);
    }

    if (not hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested)) {
        performGC (s, oldGenBytesRequested, nurseryBytesRequested, FALSE,
                   TRUE);
        if (DEBUG) {
            fprintf (stderr,
                     "%d] Back after GCin and going to check assert. oldgen size=%d\n",
                     PTHREAD_NUM, s->heap.oldGenSize);
            displayHeap (s, &(s->heap), stderr);
            assert (s->stackBottom[PTHREAD_NUM] ==
                    getStackBottom (s, getStackCurrent (s)));
        }
    }
    assert (hasHeapBytesFree
            (s, oldGenBytesRequested, nurseryBytesRequested));
}

