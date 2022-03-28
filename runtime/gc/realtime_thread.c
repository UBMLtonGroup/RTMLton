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
int allowedToPack[MAXPRI];

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

void Parallel_run(void);
void Parallel_run_rtems(void);

#if defined(__rtems__)
#define directive_failed(COND,MSG) do{if(COND){puts(MSG);exit(-1);}}while(0)

rtems_id            ML_mutex;
rtems_id            User_mutexes[NUM_USER_MUTEXES];
int                 current_runtime[MAXPRI], current_deadline[MAXPRI], current_period[MAXPRI];


rtems_task Periodic_task(rtems_task_argument argument);
void RTEMS_Parallel_run_wrapper(int tNum);

/*
 From the RTEMS doc

 The first time the loop is executed, the rtems_rate_monotonic_period directive will
 initiate the period for 100 ticks and return immediately. Subsequent invocations of
 the rtems_rate_monotonic_period directive will result in the task blocking for the
 remainder of the 100 tick period. If, for any reason, the body of the loop takes more
 than 100 ticks to execute, the rtems_rate_monotonic_period directive will return the
 RTEMS_TIMEOUT status. 

 Regarding changing a period:

 https://docs.rtems.org/releases/rtems-docs-4.11.3/c-user/rate_monotonic_manager.html#manipulating-a-period

 This periodic task initially executes Parallel_run() every 10ms. 
 10 * ticks_per_sec() / 1000

 To hit 10ms, be sure to configure RTEMS so that the clock is sufficiently high resolution.
 eg.:

 #define CONFIGURE_MICROSECONDS_PER_TICK   1000 // 1 millisecond 
 #define CONFIGURE_TICKS_PER_TIMESLICE       50 // 50 milliseconds 


 Eventually, the SML code will call set_schedule. We will then cancel the 10ms period
 and reset it to what was given to set_schedule as the period parameter. If we overrun
 our period, instead of dying (as shown in the RTEMS example), we simply note it and 
 continue. 
 */

__attribute__((noreturn))
rtems_task Periodic_task(rtems_task_argument arg) {
    /* rtems_task_argument is a 32bit ptr 
     * https://docs.rtems.org/doxygen/branches/master/group__ClassicTasks.html#gaf202f985ef5a3156f29eae99a0536842
     */
    rtems_name        name;
    rtems_id          period;
    char              tnum = arg & 0xFF;
    int               prev_period = current_period[tnum];

    rtems_status_code status;
    name = rtems_build_name( 'P', 'E', 'R', tnum );
    status = rtems_rate_monotonic_create( name, &period );
    if ( status != RTEMS_SUCCESSFUL ) {
        printf( "rtems_monotonic_create failed with status of %d.\n", status );
        exit( 1 );
    }
    while ( 1 ) {
        int x = rtems_clock_get_ticks_since_boot();

        /* if our period has changed (via set_schedule) then we need to cancel
         * and reset our period
         */
        if (prev_period != current_period[tnum]) {
            status = rtems_rate_monotonic_cancel(period);
            if (status != RTEMS_SUCCESSFUL) {
                fprintf(stderr, "%d] "RED("rtems failed to cancel current period. status=%d\n"), PTHREAD_NUM, status);
                exit(1);
            }
            prev_period = current_period[tnum];
        }

        /* the following is an implicit yield if we are still in the same period */

        if ( rtems_rate_monotonic_period( period, current_period[tnum] ) == RTEMS_TIMEOUT ) {
            fprintf(stderr, "%d] "RED("missed deadline/period cur: %d, clock: %d\n"), 
                    PTHREAD_NUM, current_period[tnum], x);
            status = rtems_rate_monotonic_cancel(period);
            if (status != RTEMS_SUCCESSFUL) {
                fprintf(stderr, "%d] "RED("rtems failed to cancel current period. status=%d\n"), PTHREAD_NUM, status);
                exit(1);
            }
            continue;
        }

        /* Perform some periodic actions */
        fprintf(stderr, "%d] calling into parallel_run_rtems\n", PTHREAD_NUM);
        Parallel_run_rtems();
        fprintf(stderr, "%d] back from parallel_run_rtems\n", PTHREAD_NUM);
    }
    /* NOTREACHED */
}

__attribute__((noreturn))
void RTEMS_Parallel_run_wrapper(int tNum) {
    rtems_id           task_id;
    rtems_status_code  status;

    /* create a new task, the identifier contains the thread number */
    status = rtems_task_create(
        rtems_build_name( 'T', 'A', '1', (char)tNum ),
        1,
        RTEMS_MINIMUM_STACK_SIZE,
        RTEMS_DEFAULT_MODES,
        RTEMS_DEFAULT_ATTRIBUTES,
        &task_id
    );
    directive_failed( status, "rtems_task_create of TA1" );

    // see note above about configuring the RTEMS clock
    current_period[tNum] = (int)(10 * rtems_clock_get_ticks_per_second() / 1000.0);

    /* start the task. the task will call Parallel_run() to execute
     * the SML code the way the linux code path does.
     */
    status = rtems_task_start( task_id, Periodic_task, tNum );
    directive_failed( status, "rtems_task_start of TA1" );
    while ( 1 ) {
        /* this loops forever, waking every 300 seconds, does not affect the period_task */
        status = rtems_task_wake_after( 300*rtems_clock_get_ticks_per_second() );
        directive_failed( status, "rtems_task_wake_after" );
    }
}

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
    return rtems_clock_get_ticks_since_boot(); // to be consistent with set_schedule below
//    return rtems_clock_get_uptime_nanoseconds();
}

/* for RTEMS, these parameters are clock ticks. the params are rtems_interval type,
 * which is Watchdog_interval which in turn is uint32_t but to keep the FFI prototype
 * consist (with the posix one below) we use 64_t and downcast. 
 */
/* packing param is 0, 1, or 2. determines if this thread is allowed
 * to pack during its releases. not all threads should/can pack
 * safely. only rate mono / deadline scheduled threads should pack
 * using "2" and only during their shared periods
 */
void set_schedule(int runtime, int deadline, int period, int packing) {
    if(DEBUG_THREADS)
        fprintf(stderr, "%d] "YELLOW("set_schedule")" runtime:%llu period:%llu deadline:%llu packing:%d\n", 
                PTHREAD_NUM, (uint64_t)runtime, (uint64_t)period, (uint64_t)deadline, packing);

    assert (runtime <= deadline);
    assert (deadline <= period);
    allowedToPack[PTHREAD_NUM] = packing;

    /* these will cause a schedule update at the start of the task's next period */

    current_runtime[PTHREAD_NUM] = runtime;
    current_deadline[PTHREAD_NUM] = deadline;
    current_period[PTHREAD_NUM] = period;

    return;
}

int schedule_yield(GC_state s, bool trigger_gc) {
    // See "11.3.6. Examples" here:
    // https://docs.rtems.org/releases/rtems-docs-4.11.3/c-user/rate_monotonic_manager.html#rtems-rate-monotonic-create
    // wait for next period

    /* yield/wfnp is implicit in RTEMS: when the computation returns, we go to the top 
     * of the loop and call rtems_rate_monotonic_period again. if we are still in 
     * the current period, we are suspended until a new period starts. therefore, this
     * function is a NOP.
     */
    return 0;
}

#else

/* for RT linux, these parameters are in milli-seconds


   from "man sched"

           arrival/wakeup                    absolute deadline
                |    start time                    |
                |        |                         |
                v        v                         v
           -----x--------xooooooooooooooooo--------x--------x---
                         |<-- Runtime ------->|
                |<----------- Deadline ----------->|
                |<-------------- Period ------------------->|

       The kernel requires that:

           sched_runtime <= sched_deadline <= sched_period
 */

#define NANOS_PER_MILLI 1000000
#define NANOS_PER_MICRO 1000

/* packing param is 0, 1, or 2. determines if this thread is allowed
 * to pack during its releases. not all threads should/can pack
 * safely. only rate mono / deadline scheduled threads should pack
 * using "2" and only during their shared periods
 * 
 * the parameters are in ms which is probably too coarse. you 
 * can change to NANOS_PER_MICRO. one issue is that on 32bit 
 * systems you can overflow if you go too fine. 
 */
void set_schedule(int runtime, int deadline, int period, int packing) {
    struct sched_attr attr;
    unsigned int flags = 0;

    if(DEBUG_THREADS)
        fprintf(stderr, "%d] "YELLOW("set_schedule")" runtime:%llu period:%llu deadline:%llu packing:%d\n", 
                PTHREAD_NUM, (uint64_t)runtime, (uint64_t)period, (uint64_t)deadline, packing);

    allowedToPack[PTHREAD_NUM] = packing;

    attr.size = sizeof(attr);
    attr.sched_flags = 0; // SCHED_FLAG_DL_OVERRUN; // supposed to make linux SIGXCPU on overrun
    attr.sched_nice = 0;
    attr.sched_priority = 0;

    /* times are in nanos for rtlinux */
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_runtime = (uint64_t)runtime * NANOS_PER_MICRO;
    attr.sched_period  = (uint64_t)period * NANOS_PER_MICRO;
    attr.sched_deadline = (uint64_t)deadline * NANOS_PER_MICRO;

    assert (runtime <= deadline);
    assert (deadline <= period);

    // '0' means 'current thread' 
    if (sched_setattr(0, &attr, flags) < 0) {
        perror("sched_setattr");
        die("realtime_thread.c set_schedule(): sched_setattr");
    }
}

int schedule_yield(GC_state s, bool trigger_gc) {
    if (sched_yield() < 0) {
        perror("sched_yield");
        die("realtime_thread.c set_schedule(): sched_yield");
    }
    if (trigger_gc) {
        GC_collect(s, 0, true, true);  // TODO not safe here
    }
    return 0;
}

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
#define MAX_INSTRUMENT 4096

unsigned int instrument_offset[MAXPRI+1];
double instrument_buffer[MAXPRI+1][MAX_INSTRUMENT];

void User_instrument (Int32 icode) {
    if (instrument_offset[PTHREAD_NUM] > (MAX_INSTRUMENT-2)) {
        fprintf(stderr, "%d] "RED("*** instrument buffer wrapped ***")"\n", 
                PTHREAD_NUM);
    }
    instrument_buffer[PTHREAD_NUM][instrument_offset[PTHREAD_NUM]] = get_ticks_since_boot();
    instrument_buffer[PTHREAD_NUM][instrument_offset[PTHREAD_NUM]+1] = (double)icode;
    instrument_offset[PTHREAD_NUM] = (instrument_offset[PTHREAD_NUM]+2) % MAX_INSTRUMENT;
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


    state->rtSync[PTHREAD_NUM] = false; // this may need to be true if "@MLton rtthreads" is false

#if defined(__rtems__)
    fprintf (stderr, "%d] "PURPLE("calling RTEMS_Parallel_run_wrapper\n"), tNum);
    RTEMS_Parallel_run_wrapper(params->tNum);
#else
    fprintf (stderr, "%d] "PURPLE("calling parallel_run\n"), tNum);
    Parallel_run ();
#endif
    fprintf (stderr, "%d] "RED("back from Parallel_run (shouldnt happen)\n"), tNum);
    exit (-1); 
	/*NOTREACHED*/
}

pointer
FFI_getOpArgsResPtr (GC_state s)
{
    return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
