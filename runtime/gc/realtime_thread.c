/* http://stackoverflow.com/questions/3649281/how-to-increase-thread-priority-in-pthreads */

#include "realtime_thread.h"

#define LOCK(X) { MYASSERT(int, pthread_mutex_lock(&X), ==, 0); }
#define UNLOCK(X) { MYASSERT(int, pthread_mutex_unlock(&X), ==, 0); }

static volatile int initialized = 0;

extern void Copy_globalObjptrs (int f, int t);


int32_t
GC_myPriority ( __attribute__ ((unused)) GC_state s)
{
    return PTHREAD_NUM;
}

int32_t
GC_threadYield ( __attribute__ ((unused)) GC_state s)
{
    fprintf (stderr, "GC_threadYield()\n");
    sched_yield ();
    return 0;
}

void
realtimeThreadWaitForInit (void)
{
    while (initialized < MAXPRI)
      {
	  fprintf (stderr, "spin [thread boot: %d out of %d]..\n",
		   initialized, MAXPRI);
      }
}

void
realtimeThreadInit (struct GC_state *state, pthread_t * main, pthread_t * gc)
{
    int rv = 0;

    state->realtimeThreads[0] = main;
    state->realtimeThreads[1] = gc;
    initialized = 2;
    // 0 = running, 1 = paused, 2 = not-ready
    state->threadPaused[0] = 0;	// main is implicitly already running
    state->threadPaused[1] = 0;	// GC we can set to running but is moot because we will never pause it

    int tNum;
    for (tNum = 2; tNum < MAXPRI; tNum++) {
	  if (DEBUG)
	      fprintf (stderr, "spawning thread %d\n", tNum);

	  struct realtimeRunnerParameters *params =
	      malloc (sizeof (struct realtimeRunnerParameters));

	  params->tNum = tNum;
	  params->state = state;

	  pthread_t *pt = malloc (sizeof (pthread_t));
	  memset (pt, 0, sizeof (pthread_t));

	  if (pthread_create (pt, NULL, &realtimeRunner, (void *) params)) {
		fprintf (stderr, "pthread_create failed: %s\n", strerror (errno));
		exit (-1);
	  }
	  else {
    	TC_LOCK;
    	TC.running_threads ++;
    	TC_UNLOCK;
    	state->realtimeThreads[tNum] = pt;
		initialized++;
	   }
    }
    state->isRealTimeThreadInitialized = TRUE;
}

#define COPYIN(s,EL) s->EL[2] = s->EL[0]

void *
realtimeRunner (void *paramsPtr)
{
    struct realtimeRunnerParameters *params = paramsPtr;
    struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num (params->tNum);

    assert (params->tNum == PTHREAD_NUM);

    while (!(state->callFromCHandlerThread != BOGUS_OBJPTR))
      {
	  if (DEBUG)
	    {
		fprintf (stderr,
			 "%d] spin [callFromCHandlerThread boot] ..\n", tNum);

	    }

	  /*If any thread has already requested a GC, the current thread can reach a GC safe point by calling perform GC itself,
	   * since it is only spinning and waiting to be linked to an SML computation. Calling the performGC function
	   * allows the current thread to systematically pause itself, although it doesnt need to GC and can be paused at any instant. */
	  if (state->GCRequested)
	    {

                if (DEBUG) 
		    fprintf (stderr,
			 "%d] Other thread requested GC. Moving to safe point. \n",
			 tNum);
		//call performGC with the state of prev executing thread as current thread has no computation
		performGC (state, state->oldGenBytesRequested,
			   state->nurseryBytesRequested, state->forceMajor,
			   state->mayResize);
	    }
	  ssleep (1, 0);
      }

    fprintf (stderr, "%d] callFromCHandlerThread %x is ready\n", tNum,
	     state->callFromCHandlerThread);

    /* state->currentThread objptr
       curct->stack         objptr

       ref: https://github.com/UBMLtonGroup/MLton/blob/master/runtime/gc/switch-thread.c#L14-L16

       given an objptr, to get GC_thread 

       thread = (GC_thread)(objptrToPointer (op, s->heap.start)
       + offsetofThread (s));

       given a pointer, to get objptr 

       stack = (GC_stack)objptrToPointer (thread->stack, s->heap.start))

       from is a GC_thread in the following.
       (objptr) s->savedThread = pointerToObjptr((pointer)from - offsetofThread (s), s->heap.start);

       pointer GC_copyThread (GC_state s, pointer p) 

     */

    /* set currentThread of new RT thread to that of main thread until copied thread is setup */
    state->currentThread[PTHREAD_NUM] = state->currentThread[0];
    setGCStateCurrentThreadAndStack (state);

    GC_thread curct =
	(GC_thread) (objptrToPointer
		     (state->currentThread[0],
		      state->heap.start) + offsetofThread (state));
    GC_stack curstk =
	(GC_stack) objptrToPointer (curct->stack, state->heap.start);

    /* GC_thread copyThread (GC_state s, GC_thread from, size_t used) */

    pointer copiedTh = GC_copyThread (state,
				      objptrToPointer (state->
						       currentThread[0],
						       state->heap.start));

    GC_thread tc = (GC_thread) (copiedTh + offsetofThread (state));
    tc->exnStack = -1;

    //current thread on for RT thread is taken from tc which is copied from main thread in previous line
    GC_switchToThread (state, tc, 0);

    state->threadPaused[params->tNum] = 0;

    COPYIN (state, savedThread);
    COPYIN (state, signalHandlerThread);
    COPYIN (state, ffiOpArgsResPtr);

    state->isRealTimeThreadRunning = TRUE;

    while (1)
      {
	  if (DEBUG)
	    {
		fprintf (stderr, "%d] realtimeRunner running.\n", tNum);
		fprintf (stderr, "%d] calling Parallel_run..\n", tNum);
	    }
	  Copy_globalObjptrs (0, params->tNum);
	  Parallel_run ();
	  fprintf (stderr, "%d] back from Parallel_run (shouldnt happen)\n",
		   tNum);
	  exit (-1);
      }
}

pointer
FFI_getOpArgsResPtr (GC_state s)
{
    return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
