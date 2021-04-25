/* http://stackoverflow.com/questions/3649281/how-to-increase-thread-priority-in-pthreads */
/* indent -nut -br -nce -cdw -i4  */

#include "realtime_thread.h"


#define LOCK_RT_THREADS IFED(pthread_mutex_lock(&state->rtThreads_lock))
#define UNLOCK_RT_THREADS IFED(pthread_mutex_unlock(&state->rtThreads_lock))
#define SIGNAL_RT_THREADS IFED(pthread_cond_signal(&state->rtThreads_cond))
#define BLOCK_RT_THREADS IFED(pthread_cond_wait(&state->rtThreads_cond,&state->rtThreads_lock))

#define BROADCAST_RT_THREADS IFED(pthread_cond_broadcast(&state->rtThreads_cond))
#define CONCURRENT
//#define DEBUG true

static volatile int initialized = 0;
volatile bool rtinitfromML = FALSE;

extern void Copy_globalObjptrs (int f, int t);

/* TID of holding thread or -1 if no one*/
volatile int32_t ML_mutex;
volatile int32_t *User_mutexes;

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

int32_t 
GC_setBooted(int32_t thr_num, GC_state s) {
    if (DEBUG_THREADS) fprintf(stderr, "%d] setBooted thr=%d\n", PTHREAD_NUM, thr_num);
    TC.booted = 1;
    s->mainBooted=TRUE;
    return 0;
}

int32_t 
GC_safePoint(int32_t thr_num) {
    if (DEBUG) fprintf(stderr, "%d] goto safepoint thr=%d\n", PTHREAD_NUM, thr_num);
    ENTER_SAFEPOINT;
    sched_yield ();
    LEAVE_SAFEPOINT;


    return 0;
}



void ML_lock (void) {

  do {
  AGAIN:
    //maybeWaitForGC (s);
    if (ML_mutex >= 0)
      goto AGAIN;
  } while (not __sync_bool_compare_and_swap (&ML_mutex,
                                             -1,
                                             PTHREAD_NUM));
  /* 
  if (needGCTime (s))
    stopTiming (&ru_lock, &s->cumulativeStatistics->ru_lock);
  */
}

void ML_unlock (void) {

  //fprintf (stderr, "unlock %d\n", Parallel_holdingMutex);

  if (not __sync_bool_compare_and_swap (&ML_mutex,
                                        PTHREAD_NUM,
                                        -1)) {
    fprintf (stderr, "ML-LOCK: can't unlock if you don't hold the lock\n");
  }
}


void User_lock (Int32 p){

    do {
        AGAIN:
            if (User_mutexes[p] >= 0)
                goto AGAIN;
       } while (not __sync_bool_compare_and_swap (&User_mutexes[p],
                                             -1,
                                             PTHREAD_NUM));
}

void User_unlock (Int32 p) {

  //fprintf (stderr, "unlock %d\n", Parallel_holdingMutex);

  if (not __sync_bool_compare_and_swap (&User_mutexes[p],
                                        PTHREAD_NUM,
                                        -1)) {
    fprintf (stderr, "USER-LOCK: can't unlock if you don't hold the lock\n");
  }
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

    state->realtimeThreads[0] = main;
    state->realtimeThreads[1] = gc;
    initialized = 2;

    int tNum;
    for (tNum = 2; tNum < MAXPRI; tNum++) {
        if (DEBUG_THREADS)
            fprintf (stderr, "spawning posix thread %d\n", tNum);

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

/*Will be called by the main thread*/
void RT_init (GC_state state)
{
    if(DEBUG_THREADS)
        fprintf(stderr, "%d] "RED("RT_init")"\n", PTHREAD_NUM);

    //fprintf(stderr, "%d] "FMTPTR" "FMTPTR" \n",
	//		PTHREAD_NUM,
	//		state->currentThread[PTHREAD_NUM],
	//		state->savedThread[PTHREAD_NUM]);

	assert (state->callFromCHandlerThread[PTHREAD_NUM] != BOGUS_OBJPTR);
	assert (state->currentThread[PTHREAD_NUM] != BOGUS_OBJPTR);
	//assert (state->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);
	state->savedThread[PTHREAD_NUM] = BOGUS_OBJPTR;

if(1)
    for(int i = 2 ; i < MAXPRI ; i++)
    {
		//fprintf(stderr, "%d] %s copy cfch thread\n", PTHREAD_NUM, __FUNCTION__);
        pointer cpThread = GC_copyThread (state,
										  objptrToPointer(state->callFromCHandlerThread[PTHREAD_NUM],
                                          state->umheap.start));
        state->callFromCHandlerThread[i] = pointerToObjptr(cpThread, state->umheap.start);


		pointer curThread = GC_copyThread (state,
										   objptrToPointer(state->currentThread[PTHREAD_NUM],
                                           state->umheap.start));

        state->currentThread[i] = pointerToObjptr(curThread, state->umheap.start);

		GC_thread t = (GC_thread)(state->currentThread[PTHREAD_NUM]);
		GC_UM_Chunk frame = (GC_UM_Chunk)(t->currentFrame - GC_HEADER_SIZE);
		frame = frame->prev_chunk;
		//fprintf(stderr, "frame for %d is "FMTPTR" thread "FMTPTR"\n", i, (uintptr_t)frame, (uintptr_t) t);
		t->currentFrame = (objptr)(frame + GC_HEADER_SIZE);
		state->currentFrame[i] = t->currentFrame;

        //state->currentFrame[i] = ((GC_thread) (curThread + offsetofThread(state)))->currentFrame;
		state->savedThread[i] = BOGUS_OBJPTR;
    }
    
    ML_mutex = -1;
    /*10 -- arbitrary number of locks the user can create through ML program*/
    User_mutexes = (int32_t *) malloc (10 * sizeof (int32_t));
    
    for(int i=0;i< 10;i++)
    {
        User_mutexes[i] = -1;
    }
    rtinitfromML = TRUE;

    
	LOCK_RT_THREADS;
	BROADCAST_RT_THREADS;
	UNLOCK_RT_THREADS;
}

Int32 RTThread_maxpri (void)
{
    return MAXPRI;
}

Int32 RTThread_get_pthread_num(void)
{
    return PTHREAD_NUM;
}


#define COPYIN2(s,EL) s->EL[2] = s->EL[0]

void Parallel_run(void);

__attribute__((noreturn))
void *
realtimeRunner (void *paramsPtr)
{
    struct realtimeRunnerParameters *params = paramsPtr;
    struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num (params->tNum);
      
    state->rtSync[PTHREAD_NUM]= true;

    LOCK_RT_THREADS;
    while(!rtinitfromML)
    {
        /*This will be unblocked in rtInit */
        if(DEBUG)
            fprintf(stderr,"%d] callFromCHandlerThread is not set, Blocking RT-Thread \n",tNum);
        BLOCK_RT_THREADS;
        UNLOCK_RT_THREADS;
    }


 
     fprintf (stderr, "%d] calling parallel_run \n", tNum);
     state->rtSync[PTHREAD_NUM]= true;
     Parallel_run ();
     fprintf (stderr, "%d] back from Parallel_run (shouldnt happen)\n",
                 tNum);
        exit (-1); 
#if 0
    /*Using same lock to BLOCK again. This time it wont be unblocked. 
     * TODO: Define what RT threads should do*/


    /* RT thread allocates on UM heap without stack*/
    if(state->numAllocedByRT <= 0)
    {
    if(DEBUG)
            fprintf(stderr,"%d] Blocking RT-Thread.FOREVA.\n",tNum);
        
       LOCK_RT_THREADS;

        while(state->rtSync[PTHREAD_NUM])
            BLOCK_RT_THREADS;

        UNLOCK_RT_THREADS; 
    }

    if (DEBUG)
     fprintf(stderr,"%d] RT thread ALLOCATING\n",tNum);

	while(1)//state->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR)
	{
		pointer res;
		state->rtSync[PTHREAD_NUM]= true;
		if(DEBUG_THREADS)
		    fprintf(stderr,"%d] Spinning with no green thread. Free chunks = %d, RTSync = %d \n",PTHREAD_NUM,state->fl_chunks,state->rtSync[PTHREAD_NUM]?1:0);

		reserveAllocation(state,state->numAllocedByRT);

		res = UM_Object_alloc(state,state->numAllocedByRT,(GC_header)3,GC_NORMAL_HEADER_SIZE);

		state->allocedByRT+=state->numAllocedByRT;

		if(DEBUG_THREADS)
		    fprintf(stderr, "Empty chunk: "FMTPTR" \n", (uintptr_t) res);

		sched_yield();

	}
#endif

#ifdef THREADED
#pragma message "*********   THREADED enabled    **********"
	die("wrong");
    while (!TC.booted) {
        if (DEBUG_THREADS) fprintf (stderr, "%d] TC.booted is false: spin\n", PTHREAD_NUM);
        ssleep (1, 0);
    }

    /* set currentThread of new RT thread to that of main thread until copied thread is setup */
    state->currentThread[PTHREAD_NUM] = state->currentThread[0];
    setGCStateCurrentThreadAndStack (state);

    GC_thread curct = (GC_thread) (objptrToPointer (state->currentThread[0],
                                                    state->heap.start) +
                                   offsetofThread (state));
    GC_stack curstk =
        (GC_stack) objptrToPointer (curct->stack, state->heap.start);

    /* GC_thread copyThread (GC_state s, GC_thread from, size_t used) */
    /* copy the savedThread which is stored earlier on from the copied thread, when C Handler was set */

    if (DEBUG)
        fprintf (stderr, "%d] copy thread\n", PTHREAD_NUM);
    pointer copiedTh = GC_copyThread (state,
                                      objptrToPointer (state->
                                                       savedThread[0],
                                                       state->heap.start));

    GC_thread tc = (GC_thread) (copiedTh + offsetofThread (state));
    tc->exnStack = -1;

    //current thread on for RT thread is taken from tc which is copied from main thread in previous line
    if (DEBUG)
        fprintf (stderr, "%d] switch to copied thread\n", PTHREAD_NUM);

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
//        Copy_globalObjptrs (0, params->tNum);

        TC_LOCK;
        TC.running_threads++;
        TC_UNLOCK;

        Parallel_run ();

        fprintf (stderr, "%d] back from Parallel_run (shouldnt happen)\n",
                 tNum);
        exit (-1);
    }
#else
#pragma message "*********   THREADED NOT enabled    **********"
#endif

	/*NOTREACHED*/
	/* since the above is wrapped in while(1) this code should never
	 * be reached and is only here to suppress gcc 'noreturn function
	 * returns' compiler warnings (since we use -Wall)
	 */
	while (1)
		fprintf (stderr, "%d] Should not get here. Spinning.\n",
				 tNum);
}

pointer
FFI_getOpArgsResPtr (GC_state s)
{
    return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
