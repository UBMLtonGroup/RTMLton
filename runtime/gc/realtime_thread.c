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
    while (initialized < MAXPRI) {
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
		    state->realtimeThreads[tNum] = pt;
		    initialized++;
		}
    }
    state->isRealTimeThreadInitialized = TRUE;
}

#define COPYIN2(s,EL) s->EL[2] = s->EL[0]

void *
realtimeRunner (void *paramsPtr)
{
    struct realtimeRunnerParameters *params = paramsPtr;
    struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num (params->tNum);

    while (!(state->callFromCHandlerThread != BOGUS_OBJPTR)) {
	if (DEBUG) {
	    fprintf (stderr,
		     "%d] spin [callFromCHandlerThread boot] ..\n", tNum);
	}
	ssleep (0, 0.1);
    }

    if (DEBUG)
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

    GC_thread curct = (GC_thread) (objptrToPointer (state->currentThread[0],
						    state->heap.start) +
				   offsetofThread (state));
    GC_stack curstk =
	(GC_stack) objptrToPointer (curct->stack, state->heap.start);

    /* GC_thread copyThread (GC_state s, GC_thread from, size_t used) */

    if (DEBUG) fprintf(stderr, "%d] copy thread\n", PTHREAD_NUM);
    pointer copiedTh = GC_copyThread (state,
				      objptrToPointer (state->currentThread[0],
						       state->heap.start));

    GC_thread tc = (GC_thread) (copiedTh + offsetofThread (state));
    tc->exnStack = -1;

    //current thread on for RT thread is taken from tc which is copied from main thread in previous line
    if (DEBUG) fprintf(stderr, "%d] switch to copied thread\n", PTHREAD_NUM);

    GC_switchToThread (state, tc, 0);

    COPYIN2 (state, savedThread);
    COPYIN2 (state, signalHandlerThread);
    COPYIN2 (state, ffiOpArgsResPtr);

    state->isRealTimeThreadRunning = TRUE;

    while (1) {
	if (DEBUG) {
	    fprintf (stderr, "%d] realtimeRunner running.\n", tNum);
	    fprintf (stderr, "%d] calling Parallel_run..\n", tNum);
	}
	Copy_globalObjptrs (0, params->tNum);

	TC_LOCK;
	TC.running_threads++;
	TC_UNLOCK;
        TC.booted = 1;

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
