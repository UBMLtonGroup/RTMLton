/* Copyright (C) 2009-2010,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void minorGC (GC_state s) {
  minorCheneyCopyGC (s);
}

void majorGC (GC_state s, size_t bytesRequested, bool mayResize) {
  uintmax_t numGCs;
  size_t desiredSize;

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
  assert (hasHeapBytesFree (s, sizeofStackWithHeader (s, reserved), 0));
  stack = newStack (s, reserved, TRUE);
  copyStack (s, getStackCurrent(s), stack);
  getThreadCurrent(s)->stack = pointerToObjptr ((pointer)stack, s->heap.start);
  markCard (s, objptrToPointer (getThreadCurrentObjptr(s), s->heap.start));
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

pthread_mutex_t gclock;
static volatile int sem;

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

#define GCLOCK(X) { MYASSERT(int, pthread_mutex_lock(&gclock), ==, 0); }
#define GCUNLOCK(X) { MYASSERT(int, pthread_mutex_unlock(&gclock), ==, 0); }

__attribute__((noreturn))
void *GCrunner(void *_s) {
	GC_state s = (GC_state) _s;

	DBG((stderr, "\t%x] GC_Runner Thread running.\n", pthread_self()));

	s->GCrunnerRunning = TRUE;
	sem = 0;

	/* we create, and then immediately lock the mutex in Initialize
	 * prior to starting this threads. The next GCLOCK should
	 * hang this thread.
	 */
#ifdef THREADED
	while (1) {
		DBG((stderr, "\t%x] GCrunner: locking mutex %x\n",
				pthread_self(), &gclock));
		GCLOCK(s);
		DBG((stderr, "\t%x] GCrunner: got lock\n", pthread_self()));

		if (sem == 0) {
			performGC_helper(s,
					s->oldGenBytesRequested,
					s->nurseryBytesRequested,
					s->forceMajor,
					s->mayResize);
			sem = 1;
		}
		else {
			DBG((stderr, "\t%x] GCrunner: skipping consecutive call to helper\n"));
		}

		DBG((stderr, "\t%x] GCrunner: unlocking mutex\n", pthread_self()));
		GCUNLOCK(s);
		DBG((stderr, "\t%x] GCrunner: unlocked (ML runs)\n", pthread_self()));
	    pthread_yield();
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
//return;

    /* In our two-thread formulation of realtime MLton, the zeroth thread runs
     * ML code, which will sometimes call the performGC function (this
     * function!). Meanwhile, the first thread repeatedly calls
     * performGC_helper with the saved parameters. We use a mutex to synchronize
     * the two threads.
     *
     * TODO When we support a 1-1 mapping between ML threads and pthreads, we
     * must revisit this method of inter-thread communication.
     */

#ifdef THREADED
    s->oldGenBytesRequested = oldGenBytesRequested;
    s->nurseryBytesRequested = nurseryBytesRequested;
    s->forceMajor = forceMajor;
    s->mayResize = mayResize;

    DBG((stderr, "%x] performGC: release mutex (GCrunner can run)\n", pthread_self()));
    GCUNLOCK(s);
    while (sem == 0) {
    	DBG((stderr, "spin.."));
    	pthread_yield();
    }

    /* GCrunner should acquire the lock and proceed */

    DBG((stderr, "%x] performGC: locking mutex\n", pthread_self()));
    GCLOCK(s);
    DBG((stderr, "%x] performGC: mutex locked; ML resumes\n", pthread_self()));
    sem = 0;

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
  size_t stackBytesRequested;
  struct rusage ru_start;
  size_t totalBytesRequested;

  DBG((stderr, "\t\t%x] in performGC_helper\n", pthread_self()));

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
  stackTopOk = invariantForMutatorStack (s);
  stackBytesRequested = 
    stackTopOk 
    ? 0 
    : sizeofStackWithHeader (s, sizeofStackGrowReserved (s, getStackCurrent (s)));
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
  unless (stackTopOk)
    growStackCurrent (s);
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
  if (force
      or not (invariantForMutatorFrontier(s))
      or not (invariantForMutatorStack(s))) {
    /* This GC will grow the stack, if necessary. */
    performGC (s, 0, getThreadCurrent(s)->bytesNeeded, force, TRUE);
  }
  assert(invariantForMutatorFrontier(s));
  assert(invariantForMutatorStack(s));
}

/* ensureHasHeapBytesFree (s, oldGen, nursery) 
 */
void ensureHasHeapBytesFree (GC_state s, 
                             size_t oldGenBytesRequested,
                             size_t nurseryBytesRequested) {
  assert (s->heap.nursery <= s->limitPlusSlop);
  assert (s->frontier <= s->limitPlusSlop);
  if (not hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested))
    performGC (s, oldGenBytesRequested, nurseryBytesRequested, FALSE, TRUE);
  assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
}

void GC_collect (GC_state s, size_t bytesRequested, bool force) {
	//fprintf(stderr, "GC_collect called from %d\n", PTHREAD_NUM);
	//return;
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
