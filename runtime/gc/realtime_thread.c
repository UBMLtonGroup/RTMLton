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
    
    for(int i =2 ; i< MAXPRI;i++)
    {
        pointer cpThread = GC_copyThread (state, objptrToPointer(state->currentThread[PTHREAD_NUM],
                                          state->umheap.start));
        state->currentThread[i] = pointerToObjptr(cpThread, state->umheap.start);

        state->currentFrame[i] = ((GC_thread) (cpThread + offsetofThread(state)))->currentFrame;
    }

    rtinitfromML = TRUE;

    
//	LOCK_RT_THREADS;
//	BROADCAST_RT_THREADS;
//	UNLOCK_RT_THREADS;
}

#define COPYIN2(s,EL) s->EL[2] = s->EL[0]

void *
realtimeRunner (void *paramsPtr)
{
    struct realtimeRunnerParameters *params = paramsPtr;
    struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num (params->tNum);
      
    state->rtSync[PTHREAD_NUM]= true;


    /*LOCK_RT_THREADS;
    while(!(state->callFromCHandlerThread != BOGUS_OBJPTR))
    {
        *This will be unblocked in GC_setcallFromCHandlerThread *
        if(DEBUG)
            fprintf(stderr,"%d] callFromCHandlerThread is not set, Blocking RT-Thread \n",tNum);
        BLOCK_RT_THREADS;
        UNLOCK_RT_THREADS;
    }
     if (DEBUG)
        fprintf (stderr, "%d] callFromCHandlerThread %x is ready\n", tNum,
                 state->callFromCHandlerThread);

    */

   // LOCK_RT_THREADS;
    while(!rtinitfromML)
    {
        /*This will be unblocked in rtInit */
        if(DEBUG)
            fprintf(stderr,"%d] callFromCHandlerThread is not set, Blocking RT-Thread \n",tNum);
       // BLOCK_RT_THREADS;
       // UNLOCK_RT_THREADS;
    }


 
     fprintf (stderr, "%d] calling parallel_run \n", tNum);
     state->rtSync[PTHREAD_NUM]= true;
     Parallel_run ();
   
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

   /* fprintf(stderr,"%d] RT thread\n",PTHREAD_NUM)   ;

    GC_thread rtTH = (GC_thread) (objptrToPointer (state->savedThread[PTHREAD_NUM], state->heap.start) +
                                   offsetofThread (state));

    state->currentThread[PTHREAD_NUM] = state->savedThread[PTHREAD_NUM];
    setGCStateCurrentThreadAndStack (state);

    GC_switchToThread(state,rtTH,0);
    */

#ifdef THREADED
    while (!TC.booted) {
        if (DEBUG_THREADS) fprintf (stderr, "%d] TC.booted is false: spin\n", PTHREAD_NUM);
        ssleep (1, 0);
    }

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

#endif
}

pointer
FFI_getOpArgsResPtr (GC_state s)
{
    return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
