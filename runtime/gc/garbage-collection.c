/* Copyright (C) 2009-2010,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#include <stdlib.h>

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

pthread_mutex_t gcflag_lock;
static volatile int gcflag = -1;
static volatile int GCRequestedBy = -1;

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

#define COPYIN(EL) s->EL[1] = s->EL[GCRequestedBy]
#define COPYOUT(EL) s->EL[GCRequestedBy] = s->EL[1]
#define SANITY(EL) if (s->EL[GCRequestedBy] == s->EL[1]) fprintf(stderr,"%d] " #EL " changed!\n",PTHREAD_NUM);

static void setup_for_gc(GC_state s) {
    assert(gcflag != 1);
    COPYIN(stackTop);
    fprintf(stderr,"%d] GCREqBy = %d , before copy stackBottom = %"PRIuMAX" , should become = %"PRIuMAX" , actually = %"PRIuMAX" \n",PTHREAD_NUM,GCRequestedBy,s->stackBottom[1],s->stackBottom[GCRequestedBy],s->stackBottom[0]);
    COPYIN(stackBottom);
    fprintf(stderr,"%d] GCReqBy= %d,  after copy StackBottom = %"PRIuMAX" \n",PTHREAD_NUM,GCRequestedBy,s->stackBottom[1]);
    COPYIN(stackLimit);
    COPYIN(exnStack);
    fprintf(stderr,"%d] GCREqBy = %d , before copy currentThread = %"PRIuMAX" , should become = %"PRIuMAX" , actually = %"PRIuMAX" \n",PTHREAD_NUM,GCRequestedBy,s->currentThread[1],s->currentThread[GCRequestedBy],s->currentThread[0]);
    COPYIN(currentThread);
    fprintf(stderr,"%d] GCReqBy= %d,  after copy currentThread = %"PRIuMAX" \n",PTHREAD_NUM,GCRequestedBy,s->currentThread[1]);
    COPYIN(savedThread);
    fprintf(stderr,"%d] GCReqBy= %d,  after copy currentThread = %"PRIuMAX" \n",PTHREAD_NUM,GCRequestedBy,s->currentThread[1]);
    COPYIN(signalHandlerThread);
    COPYIN(ffiOpArgsResPtr);
    gcflag=PTHREAD_NUM;
}

static void sanity_check_array(GC_state s) {
    assert(gcflag != -1 && gcflag != 1);
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
    assert(gcflag != -1 && gcflag != 1);
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


#define LOCKFLAG MYASSERT(int, pthread_mutex_lock(&gcflag_lock), ==, 0)
#define UNLOCKFLAG MYASSERT(int, pthread_mutex_unlock(&gcflag_lock), ==, 0)
#define PAUSESELF do {s->threadPaused[PTHREAD_NUM]=1;s->GCRequested=TRUE;GCRequestedBy =PTHREAD_NUM;push(s,PTHREAD_NUM); pthread_kill(pthread_self(),SIGUSR1);} while(0)
#define REQUESTGC do { LOCKFLAG; setup_for_gc(s); UNLOCKFLAG;PAUSESELF; } while(0)
#define COMPLETEGC do { LOCKFLAG;finish_for_gc(s); gcflag = -1; UNLOCKFLAG;s->GCRequested=FALSE; } while(0)
/*
 * pthread_signal(pthread_self(), SIGUSR1);
 */
__attribute__((noreturn))
void *GCrunner(void *_s) {
	GC_state s = (GC_state) _s;

	set_pthread_num(1); // by definition

	if (DEBUG)
		fprintf(stderr, "%d] GC_Runner Thread running.\n", PTHREAD_NUM);

	s->GCrunnerRunning = TRUE;

#ifdef THREADED
	while (1) {
		if (DEBUG)
			fprintf(stderr, "%d] GCrunner: spinning\n", PTHREAD_NUM);

		while (gcflag == -1) {
			sched_yield();
		}

		if (DEBUG)
			fprintf(stderr, "%d] GCrunner: GC requested. pausing threads.\n", PTHREAD_NUM);
			
		/* if GC collect is called even before the realtime threads are initialized, then the GC runner will try to 
		 * pause threads that dont exist. Hence the quiesce_threads should be called only once the RT threads 
		 * are initialized (//TODO reasoning)
		 *
		 * but.. c-main.h does the following:
		 * 1. initialize structures
		 * 2. create gc runner
		 * 3. initialize rt threads
		 * 4. wait for rt threads to initialize
		 * 5. continue with main (thread 0)
		 *
		 * so... we already wait for threads to initialize, so this is redundant. jm
		 */
		if(s->isRealTimeThreadInitialized)
		{
			/* we wont be quiescing threads as in the new implementation, each individual thread will pause itself */ 

			//quiesce_threads(s);

			if (DEBUG)
				fprintf(stderr, "%d] GCrunner: threads paused. GC'ing\n", PTHREAD_NUM);

			//TODO need to uncomment this line and delete next line once the 
			//spinning RT thread has computation
                    /*
			if(s->isRealTimeThreadRunning)	
			s->currentThread[1] = s->currentThread[gcflag];
			else
			{
			//Always set to main thread since thread doesnt have computation yet. 
			s->currentThread[1] = s->currentThread[0];
			}
                    */
			performGC_helper(s,
					s->oldGenBytesRequested,
					s->nurseryBytesRequested,
					s->forceMajor,
					s->mayResize);

			COMPLETEGC;
			if (DEBUG)
				fprintf(stderr, "%d] GCrunner: finished. unpausing threads.\n", PTHREAD_NUM);
			/*	
				do {
					fprintf(stderr, "%d] GCrunner: resuming %d threads.\n",PTHREAD_NUM, paused_threads_count(s));
					resume_threads(s);
				} while(paused_threads_count(s));
			*/
				resume_threads(s,pop(s));
		}
		else {
			fprintf(stderr, "%d] GCrunner: skipping thread pause bc RTT is not yet initialized\n", PTHREAD_NUM);
		}
	}
#endif

	pthread_exit(NULL);
	/*NOTREACHED*/
}

bool lastUnpausedThread(GC_state s)
{

	bool res = false;
        int cnt=0;
	for(uint32_t i=0;i<MAXPRI;i++)
	{
		if(i == 1) continue; //we dont need to see if GC thread is paused
		if(!s->threadPaused[i])
			cnt++;
	}
        if(cnt == 1)
            res=true;
    
	return res;
}


bool checkAllPaused(GC_state s)
{
	bool res = true;
        if(!lastUnpausedThread(s))
        {
            for(uint32_t i=0;i<MAXPRI;i++)
        	{
        		if(i == 1) continue; //we dont need to see if GC thread is paused
        		if(!s->threadPaused[i])
        			res=false;
        	}
        }
	return res;

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
     * pause. We do this by calling quiesce_threads(s) which will
     * signal the other threads and then wait for them to indicate
     * they are paused. Once the GC finishes, it uses
     * resume_threads(s) to let the threads know they can resume.
     */

    /* EDIT: above method of GC pausing all other threads may cause 
     * the thread to be paused where it isn't safe to be paused. Thus
     * we allow the current executing thread to continue execution till
     * it reaches a GC safepoint. Once it is safe, the thread can pause itself 
     * and yield control to another thread (not GC) which will do the same
     * until the last thread is reached. The las thread, pauses itself and 
     * tells the GC thread that it can now GC and all other threads are
     * paused. 
     * */

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
    while (gcflag > -1) sched_yield();

    if(MAXPRI <=2)
    {
	    if(DEBUG)
		    fprintf(stderr,"%d] performGC: pausing main thread and calling GC\n",PTHREAD_NUM);
	    GCRequestedBy=0;
	    s->GCRequested =TRUE;
	    REQUESTGC;
	   	
    }
    else
    {

  	  //TODO handle race when RT thread has computation.
    	//TODO race when you run on multicore. GC flag is set before thread 0 is paused
    	if(checkAllPaused(s))
    	{
    		if (DEBUG)
	    		fprintf(stderr, "%d] performGC: requesting a GC\n", PTHREAD_NUM);
		REQUESTGC;
    	}
    	else
	{
	 	   if(DEBUG)
        		   fprintf(stderr,"%d] performGC: all threads not paused so pausing self\n",PTHREAD_NUM);
		   PAUSESELF;
	}
    }


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
	  fprintf(stderr, "%d] in performGC_helper\n", PTHREAD_NUM,s->currentThread[1]);

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

#if 0
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
	fprintf(stderr, "%d] ensureInvariantForMutator\n", PTHREAD_NUM);

	if (force or not (invariantForMutatorFrontier(s))) {
		performGC (s, 0, getThreadCurrent(s)->bytesNeeded, force, TRUE);
	}

	if (not (invariantForMutatorStack(s))) maybe_growstack(s);

	assert(invariantForMutatorFrontier(s));
	fprintf(stderr, "%d] ensureInvariantForMutatorStack 2nd call\n", PTHREAD_NUM);
	assert(invariantForMutatorStack(s));
}

/* ensureHasHeapBytesFree (s, oldGen, nursery) 
 */
void ensureHasHeapBytesFree (GC_state s, 
                             size_t oldGenBytesRequested,
                             size_t nurseryBytesRequested) {
  assert (s->heap.nursery <= s->limitPlusSlop);
  assert (s->frontier <= s->limitPlusSlop);
  
  displayHeap(s, &(s->heap), stderr);
   displayHeapInfo(s);

  if (not hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested))
  {
    performGC (s, oldGenBytesRequested, nurseryBytesRequested, FALSE, TRUE);
	
	fprintf(stderr, "%d] Back after GCin and going to check assert. oldgen size=%d\n", PTHREAD_NUM,s->heap.oldGenSize);
	displayHeap(s, &(s->heap), stderr);
  	assert (s->stackBottom[PTHREAD_NUM] == getStackBottom (s, getStackCurrent(s)));
	
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


