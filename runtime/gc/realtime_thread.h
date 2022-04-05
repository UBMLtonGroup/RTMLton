#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

/* maxpri is an upper array bound, not a count. see
 * realtime_thread.c realtimeThreadInit loop (~line 251)
 */

#define MAXPRI 6 /* 0 = main, 1 = GC, also in include/c-common.h */
#define PTHREAD_MAX MAXPRI  /* transitioning to this instead of MAXPRI */
#define NUM_USER_MUTEXES 10

#if (defined (MLTON_GC_INTERNAL_FUNCS))
struct realtimeRunnerParameters {
    int tNum;
    struct GC_state *state;
};

void realtimeThreadInit(struct GC_state *state, pthread_t *, pthread_t *);
void *realtimeRunner(void* paramsPtr) __attribute__((noreturn));
void realtimeThreadWaitForInit(void);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))
pointer FFI_getOpArgsResPtr (GC_state s);
int32_t GC_setBooted(int32_t thr_num,GC_state s);
int32_t GC_safePoint(int32_t thr_num);
int32_t GC_threadYield(GC_state s);
int32_t GC_myPriority(GC_state s);
void RT_init (GC_state state);
Int32 RTThread_maxpri (void);
Int32 RTThread_get_pthread_num(void);
void InitializeMutexes(void);
void ML_lock (void);
void ML_unlock (void);
void User_lock (Int32);
void User_unlock (Int32);
void User_instrument_initialize (void);
void User_instrument (Int32 p);
void Dump_instrument_stderr (Int32 p);
void User_instrument_counter (Int32 p, Int32 i);
void Dump_instrument_counter_stderr (Int32 p);
double get_ticks_since_boot(void);
void set_schedule(int runtime, int deadline, int period, int packing);
int schedule_yield(GC_state s, bool trigger_gc);
#endif

#endif /* _REALTIME_THREAD_H_ */
