#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

#define MAXPRI 3 /* 0 = main, 1 = GC, also in include/c-common.h */
#define PTHREAD_MAX MAXPRI  /* transitioning to this instead of MAXPRI */

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
void ML_lock (void);
void ML_unlock (void);
#endif

#endif /* _REALTIME_THREAD_H_ */
