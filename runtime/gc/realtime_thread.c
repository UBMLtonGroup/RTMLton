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

#if defined(__rtems__)

rtems_id            ML_mutex;
rtems_id            User_mutexes[NUM_USER_MUTEXES];

void ML_lock (void) {
    while (RTEMS_SUCCESSFUL != rtems_barrier_wait(ML_mutex, RTEMS_NO_TIMEOUT)) {
        //die("ML_lock::rtems_barrier_wait failed");
    }
}

void ML_unlock (void) {
    uint32_t released;
    if (RTEMS_SUCCESSFUL != rtems_barrier_release(ML_mutex, &released)) {
        die("ML_unlock::rtems_barrier_release failed");
    }
}

void User_lock (Int32 p) {
    while (RTEMS_SUCCESSFUL != rtems_barrier_wait(User_mutexes[p], RTEMS_NO_TIMEOUT)) {
        //die("User_lock::rtems_barrier_wait failed");
    }
}

void User_unlock (Int32 p) {
    uint32_t released;
    if (RTEMS_SUCCESSFUL != rtems_barrier_release(User_mutexes[p], &released)) {
        die("User_unlock::rtems_barrier_release failed");
    }
}

void InitializeMutexes(void) {
    if (RTEMS_SUCCESSFUL != rtems_barrier_create(
            0xDEADBEEF,
            RTEMS_BARRIER_MANUAL_RELEASE,
            MAXPRI+1,
            &ML_mutex
        )) {
            die("InitializeMutexes::rtems_barrier_create failed");
    }

    for(int i=0 ; i < NUM_USER_MUTEXES ; i++)
    {
        if (RTEMS_SUCCESSFUL != rtems_barrier_create(
            0xDEADBEF0+i,
            RTEMS_BARRIER_MANUAL_RELEASE,
            MAXPRI+1,
            &User_mutexes[NUM_USER_MUTEXES]
        )) {
            die("rtems_barrier_create failed");
        }
    }

}

double get_ticks_since_boot(void) {
    rtems_status_code sc;
    rtems_interval    time_buffer;

    sc = rtems_clock_get(RTEMS_CLOCK_GET_TICKS_SINCE_BOOT, (void *)&time_buffer);
    assert (rc == RTEMS_SUCCESSFUL);
    return (double)time_buffer;
}

#else

/* TID of holding thread or -1 if no one*/
volatile int32_t ML_mutex;
volatile int32_t *User_mutexes;


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

void InitializeMutexes(void) {
    ML_mutex = -1;
    /*10 -- arbitrary number of locks the user can create through ML program*/
    User_mutexes = (int32_t *) malloc (NUM_USER_MUTEXES * sizeof (int32_t));
    
    for(int i=0 ; i < NUM_USER_MUTEXES ; i++)
    {
        User_mutexes[i] = -1;
    }
}

/* returns seconds */
double get_ticks_since_boot(void) {
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec.tv_sec + spec.tv_nsec / 1.0e9;
}
#endif

// must be a multiple of 2*sizeof(int)
#define MAX_INSTRUMENT 1024

unsigned int instrument_offset[MAXPRI+1];
double instrument_buffer[MAXPRI+1][MAX_INSTRUMENT];

void User_instrument (Int32 icode) {
    instrument_buffer[PTHREAD_NUM][instrument_offset[MAXPRI]] = get_ticks_since_boot();
    instrument_buffer[PTHREAD_NUM][instrument_offset[MAXPRI]+1] = (double)icode;
    instrument_offset[MAXPRI] = (instrument_offset[MAXPRI]+2) % MAX_INSTRUMENT;
}

void Dump_instrument_stderr (Int32 thrnum) {
    if (thrnum == -1) thrnum = PTHREAD_NUM;

    fprintf(stderr, "thread-id, time-stamp, code-number\n");

    for(unsigned int i = 0 ; i < MAX_INSTRUMENT ; i += 2) {
        if (instrument_buffer[thrnum][i] > 0 || instrument_buffer[thrnum][i] < 0) {
            fprintf(stderr, "%d, %f, %f\n", 
                    thrnum, 
                    instrument_buffer[thrnum][i],
                    instrument_buffer[thrnum][i+1]
                    );
        }
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

    if(state->useRTThreads)
    {
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
    else
    {
        /*HAck to ensure the main thread doesnt wait for RT threads to be created*/
        initialized = MAXPRI;
    }
}

/*Will be called by the main thread*/
void RT_init (GC_state state)
{
    if(DEBUG_THREADS)
        fprintf(stderr, "%d] "RED("RT_init")"\n", PTHREAD_NUM);


    memset(instrument_offset, 0, sizeof(instrument_offset[0]) * (MAXPRI+1));
    memset(instrument_buffer, 0, sizeof(instrument_buffer[0][0]) * (MAXPRI+1) * MAX_INSTRUMENT);

    //fprintf(stderr, "%d] "FMTPTR" "FMTPTR" \n",
	//		PTHREAD_NUM,
	//		state->currentThread[PTHREAD_NUM],
	//		state->savedThread[PTHREAD_NUM]);

	assert (state->callFromCHandlerThread[PTHREAD_NUM] != BOGUS_OBJPTR);
	assert (state->currentThread[PTHREAD_NUM] != BOGUS_OBJPTR);
	//assert (state->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);
	state->savedThread[PTHREAD_NUM] = BOGUS_OBJPTR;

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
    
    InitializeMutexes();

    rtinitfromML = TRUE;

    
	LOCK_RT_THREADS;
	BROADCAST_RT_THREADS;
	UNLOCK_RT_THREADS;
}

/* be careful when using this in SML code. remember that maxpri
 * includes the main thread and the GC, and it is one more than the
 * actual final thread-id. ie. if MAXPRI is 3, then we have 
 * 
 * TID  Description
 * 0    main thr
 * 1    GC
 * 2    another thread
 */
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
      
    state->rtSync[PTHREAD_NUM] = true;

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

    state->rtSync[PTHREAD_NUM] = false; // this may need to be true if "@MLton rtthreads" is false

    Parallel_run (); 
    fprintf (stderr, "%d] back from Parallel_run (shouldnt happen)\n", tNum);
    exit (-1); 
	/*NOTREACHED*/
}

pointer
FFI_getOpArgsResPtr (GC_state s)
{
    return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
