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

struct thrctrl {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_cond_t safepoint_cond; // "paused" threads will wait on this condition
    pthread_mutex_t safepoint_lock;

    int running_threads;
    int gc_needed;
    int requested_by;
} TC;

#define TC_LOCK pthread_mutex_lock(&TC.lock)
#define TC_UNLOCK pthread_mutex_unlock(&TC.lock)
#define TCSP_LOCK pthread_mutex_lock(&TC.safepoint_lock)
#define TCSP_UNLOCK pthread_mutex_unlock(&TC.safepoint_lock)

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
        if (DEBUG) fprintf(stderr, "%d] REQUESTGC start %d\n", PTHREAD_NUM, TC.running_threads); \
        TC_LOCK; TC.gc_needed = 1; TC.requested_by = PTHREAD_NUM; setup_for_gc(s); TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] REQUESTGC end %d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define COMPLETEGC do { \
        if (DEBUG) fprintf(stderr, "%d] COMPLETEGC start %d\n", PTHREAD_NUM, TC.running_threads); \
        TC_LOCK; finish_for_gc(s); TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] COMPLETEGC end %d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define ENTER_SAFEPOINT do { \
        if (DEBUG) fprintf(stderr, "%d] ENTER_SAFEPOINT start %d\n", PTHREAD_NUM, TC.running_threads); \
        TC_LOCK; TC.running_threads--; pthread_cond_signal(&TC.cond); TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] ENTER_SAFEPOINT end %d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
#define LEAVE_SAFEPOINT do { \
        if (DEBUG) fprintf(stderr, "%d] LEAVE_SAFEPOINT start %d\n", PTHREAD_NUM, TC.running_threads); \
        TCSP_LOCK; while (TC.gc_needed) pthread_cond_wait(&TC.safepoint_cond, &TC.safepoint_lock); TCSP_UNLOCK; \
        TC_LOCK; TC.running_threads++; TC_UNLOCK; \
        if (DEBUG) fprintf(stderr, "%d] LEAVE_SAFEPOINT end %d\n", PTHREAD_NUM, TC.running_threads); \
        } while(0)
        

#ifndef CHECKDISABLEGC
# define CHECKDISABLEGC do { if (getenv("DISABLEGC")) { fprintf(stderr, "GC is disabled\n"); return; } } while(0)
#endif

void minorGC (GC_state s) {
  CHECKDISABLEGC;
  minorCheneyCopyGC (s);
}

void majorGC (GC_state s, size_t bytesRequested, bool mayResize) {
  uintmax_t numGCs;
  size_t desiredSize;
  CHECKDISABLEGC;

  fprintf(stderr, "%d] [GC: Starting Major GC...]\n",PTHREAD_NUM);

  s->lastMajorStatistics.numMinorGCs = 0;
  numGCs = 
    s->cumulativeStatistics.numCopyingGCs 
    + s->cumulativeStatistics.numMarkCompactGCs;
  if (0 < numGCs
      and ((float)(s->cumulativeStatistics.numHashConsGCs) / (float)(numGCs)
           < s->controls.ratios.hashCons))
    s->hashConsDuringGC = TRUE;
  desiredSize = 
    sizeofHeapDesired (s, s->lastMajorStatistics.bytesLive + bytesRequested, 0);
  if (not FORCE_MARK_COMPACT
      and not s->hashConsDuringGC // only markCompact can hash cons
      and s->heap.withMapsSize < s->sysvals.ram
      and (not isHeapInit (&s->secondaryHeap)
           or createHeapSecondary (s, desiredSize)))
    majorCheneyCopyGC (s);
  else
    majorMarkCompactGC (s);
  s->hashConsDuringGC = FALSE;
  s->lastMajorStatistics.bytesLive = s->heap.oldGenSize;
  if (s->lastMajorStatistics.bytesLive > s->cumulativeStatistics.maxBytesLive)
    s->cumulativeStatistics.maxBytesLive = s->lastMajorStatistics.bytesLive;
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

void growStackCurrent (GC_state s) {
  size_t reserved;
  GC_stack stack;

  reserved = sizeofStackGrowReserved (s, getStackCurrent(s));
  if (DEBUG_STACKS or s->controls.messages)
    fprintf (stderr, 
             "[GC: Growing stack of size %s bytes to size %s bytes, using %s bytes.]\n",
             uintmaxToCommaString(getStackCurrent(s)->reserved),
             uintmaxToCommaString(reserved),
             uintmaxToCommaString(getStackCurrent(s)->used));

  /* TODO insufficient heap will cause grow to fail since we've now separated
   * stack ops from heap ops
   */

  if (not hasHeapBytesFree (s, sizeofStackWithHeader(s,reserved),0))
  {
        if (DEBUG_STACKS or s->controls.messages)
	    fprintf(stderr,"%d]No heap bytes free to grow stack hence calling GC\n", PTHREAD_NUM);
  //		 resizeHeap (s, s->lastMajorStatistics.bytesLive + sizeofStackWithHeader(s,reserved));
		ensureHasHeapBytesFree(s,sizeofStackWithHeader(s,reserved),0);
  }


//  assert (hasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0));
	  stack = newStack (s, reserved, TRUE);
	  copyStack (s, getStackCurrent(s), stack);
	  getThreadCurrent(s)->stack = pointerToObjptr ((pointer)stack, s->heap.start);
	  markCard (s, objptrToPointer (getThreadCurrentObjptr(s), s->heap.start));
	
	 s->stackBottom[PTHREAD_NUM] = getStackBottom(s,stack);
 	 s->stackTop[PTHREAD_NUM] = getStackTop(s, stack); 
	 s->stackLimit[PTHREAD_NUM] = getStackLimit(s, stack);
}

void maybe_growstack(GC_state s) {
	bool stackTopOk;
	size_t stackBytesRequested;
	if (isStackEmpty (getStackCurrent(s))) return;
    stackTopOk = invariantForMutatorStack (s);
    stackBytesRequested =
      stackTopOk
      ? 0
      : sizeofStackWithHeader (s, sizeofStackGrowReserved (s, getStackCurrent (s)));
    unless (stackTopOk)
      growStackCurrent (s);
}

void enterGC (GC_state s) {
  if (s->profiling.isOn) {
    /* We don't need to profileEnter for count profiling because it
     * has already bumped the counter.  If we did allow the bump, then
     * the count would look like function(s) had run an extra time.
     */  
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileEnter (s);
  }
  s->amInGC = TRUE;
}

/* TODO
 * define THREADED to enable threading (right now its linear just like
 * the original code)
 */
void leaveGC (GC_state s) {
  if (s->profiling.isOn) {
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileLeave (s);
  }
  s->amInGC = FALSE;
}

#define THREADED

#undef GCTHRDEBUG

#ifdef GCTHRDEBUG
# define DBG(X) fprintf X
#else
# define DBG(X)
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
#define SANITY(EL) if (s->EL[TC.requested_by] == s->EL[1]) fprintf(stderr,"%d] " #EL " changed!\n", PTHREAD_NUM);

static void setup_for_gc(GC_state s) {
    assert(gcflag != 1);
    COPYIN(stackTop);
    if (DEBUG) 
        fprintf(stderr,"%d] GCREqBy = %d , before copy stackBottom = %"PRIuMAX" , should become = %"PRIuMAX" , actually = %"PRIuMAX" \n",
        	PTHREAD_NUM, TC.requested_by, 
        	s->stackBottom[1], s->stackBottom[TC.requested_by], s->stackBottom[0]);
    COPYIN(stackBottom);
    if (DEBUG) 
        fprintf(stderr,"%d] GCReqBy= %d,  after copy StackBottom = %"PRIuMAX" \n",PTHREAD_NUM, TC.requested_by, s->stackBottom[1]);
    COPYIN(stackLimit);
    COPYIN(exnStack);
    if (DEBUG) 
        fprintf(stderr,"%d] GCREqBy = %d , before copy currentThread = %"FMTPTR" , should become = %"FMTPTR" , main thread = %"FMTPTR" \n",
        	PTHREAD_NUM, TC.requested_by, objptrToPointer(s->currentThread[1],s->heap.start),
        	objptrToPointer(s->currentThread[GCRequestedBy],s->heap.start),
        	objptrToPointer(s->currentThread[0], s->heap.start));
    COPYIN(currentThread);
    if (DEBUG) 
        fprintf(stderr,"%d] GCReqBy= %d,  after copy currentThread = %"FMTPTR" \n",
        	PTHREAD_NUM, TC.requested_by, objptrToPointer(s->currentThread[1], s->heap.start));
    COPYIN(savedThread);
    COPYIN(signalHandlerThread);
    COPYIN(ffiOpArgsResPtr);
}

static void sanity_check_array(GC_state s) {
    SANITY(stackTop);
    SANITY(stackBottom);
    SANITY(stackLimit);
    SANITY(exnStack);
    SANITY(currentThread);
    SANITY(savedThread);
    SANITY(signalHandlerThread);
    SANITY(ffiOpArgsResPtr);
}

static void finish_for_gc(GC_state s) {
    sanity_check_array(s);
    COPYOUT(stackTop);
    COPYOUT(stackBottom);
    COPYOUT(stackLimit);
    COPYOUT(exnStack);
    COPYOUT(currentThread);
    COPYOUT(savedThread);
    COPYOUT(signalHandlerThread);
    COPYOUT(ffiOpArgsResPtr);
}



__attribute__((noreturn))
void *GCrunner(void *_s) {
	GC_state s = (GC_state) _s;

	set_pthread_num(1); // by definition
    TC.running_threads = 0;
    TC.gc_needed = 0;
    TC.requested_by = 0;
    
    pthread_mutex_init(&TC.lock, NULL);
    pthread_mutex_init(&TC.safepoint_lock, NULL);
    pthread_cond_init(&TC.cond, NULL);
    pthread_cond_init(&TC.safepoint_cond, NULL);
    
	if (DEBUG)
		fprintf(stderr, "%d] GC_Runner Thread running.\n", PTHREAD_NUM);

	s->GCrunnerRunning = TRUE;

#ifdef THREADED
	while (1) {
		if (DEBUG)
			fprintf(stderr, "%d] GCrunner: waiting for GC request.\n", PTHREAD_NUM);

	    TC_LOCK;
	    do {
	        do {
	            pthread_cond_wait(&TC.cond, &TC.lock);  // implicit TC_UNLOCK
	        } while (TC.running_threads);
	    } while (!TC.gc_needed);

		if (DEBUG)
			fprintf(stderr, "%d] GCrunner: GC requested. all threads should be paused.\n", PTHREAD_NUM);
	    
		// at this point, all threads should be paused and the GC can proceed    

		if(s->isRealTimeThreadInitialized)
		{
			if (DEBUG) {
				fprintf(stderr, "%d] GCrunner: threads paused. GC'ing\n", PTHREAD_NUM);
				fprintf(stderr, "%d] GC running needed=%d threads=%d\n", PTHREAD_NUM, 
						TC.gc_needed, TC.running_threads);
			}

			performGC_helper(s,
					s->oldGenBytesRequested,
					s->nurseryBytesRequested,
					s->forceMajor,
					s->mayResize);

			COMPLETEGC;
			
			if (DEBUG)
				fprintf(stderr, "%d] GCrunner: finished. unpausing threads.\n", PTHREAD_NUM);

        	TC.gc_needed = 0;
        	pthread_cond_broadcast(&TC.safepoint_cond); // unpause all threads

        	TC_UNLOCK;
		}
		else {
			fprintf(stderr, "%d] GCrunner: skipping thread pause bc RTT is not yet initialized\n", PTHREAD_NUM);
		}
	}
#endif

	pthread_exit(NULL);
	/*NOTREACHED*/
}

void performGC (GC_state s,
                size_t oldGenBytesRequested,
                size_t nurseryBytesRequested,
                bool forceMajor,
                bool mayResize) {
    if (DEBUG)
    	fprintf(stderr, "%d] performGC: starting..\n", PTHREAD_NUM);

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
    		fprintf(stderr, "%d] spin [can't performGC yet] ..\n", PTHREAD_NUM);
    	}
    	ssleep(1, 0);
    }
    
    REQUESTGC;
	ENTER_SAFEPOINT;
	sched_yield();
	LEAVE_SAFEPOINT;

#else
    DBG((stderr, "non-threaded mode, passing thru to performGC_helper\n"));
    performGC_helper(s, oldGenBytesRequested, nurseryBytesRequested, forceMajor, mayResize);
#endif
}

void performGC_helper (GC_state s, 
                size_t oldGenBytesRequested,
                size_t nurseryBytesRequested, 
                bool forceMajor,
                bool mayResize) {
  uintmax_t gcTime;
  bool stackTopOk;
  size_t stackBytesRequested = 0;
  struct rusage ru_start;
  size_t totalBytesRequested;

  if (DEBUG)
	  fprintf(stderr, "%d] in performGC_helper\n", PTHREAD_NUM);

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
  minorGC (s);

#if 0  /* XXX stack grow moved outside of gc */
  stackTopOk = invariantForMutatorStack (s);
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
      or totalBytesRequested > s->heap.size - s->heap.oldGenSize)
    majorGC (s, totalBytesRequested, mayResize);
  setGCStateCurrentHeap (s, oldGenBytesRequested + stackBytesRequested, 
                         nurseryBytesRequested);
  assert (hasHeapBytesFree (s, oldGenBytesRequested + stackBytesRequested,
                            nurseryBytesRequested));
#if 0  /* XXX stack grow moved outside of gc */
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
      and s->signalHandlerThread[PTHREAD_NUM] != BOGUS_OBJPTR) {
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
	if (DEBUG) fprintf(stderr, "%d] ensureInvariantForMutator\n", PTHREAD_NUM);

	if (force or not (invariantForMutatorFrontier(s))) {
		performGC (s, 0, getThreadCurrent(s)->bytesNeeded, force, TRUE);
	}

	if (not (invariantForMutatorStack(s))) maybe_growstack(s);

	assert(invariantForMutatorFrontier(s));
	if (DEBUG) fprintf(stderr, "%d] ensureInvariantForMutatorStack 2nd call\n", PTHREAD_NUM);
	assert(invariantForMutatorStack(s));
}

/* ensureHasHeapBytesFree (s, oldGen, nursery) 
 */
void ensureHasHeapBytesFree (GC_state s, 
                             size_t oldGenBytesRequested,
                             size_t nurseryBytesRequested) {
  assert (s->heap.nursery <= s->limitPlusSlop);
  assert (s->frontier <= s->limitPlusSlop);
  
  if (DEBUG) {
     displayHeap(s, &(s->heap), stderr);
     displayHeapInfo(s);
  }

  if (not hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested))
  {
    performGC (s, oldGenBytesRequested, nurseryBytesRequested, FALSE, TRUE);
    if (DEBUG) {
		fprintf(stderr, "%d] Back after GCin and going to check assert. oldgen size=%d\n", PTHREAD_NUM, s->heap.oldGenSize);
		displayHeap(s, &(s->heap), stderr);
  		assert (s->stackBottom[PTHREAD_NUM] == getStackBottom (s, getStackCurrent(s)));
    }
  }
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
}

void GC_collect (GC_state s, size_t bytesRequested, bool force) {

  if (DEBUG) fprintf(stderr, "%d] GC_collect called\n", PTHREAD_NUM);
  CHECKDISABLEGC;
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
}


