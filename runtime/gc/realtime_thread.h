#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

#define MAXPRI 2 /* 0 = main, 1 = GC, also in include/c-common.h */

#if (defined (MLTON_GC_INTERNAL_FUNCS))
struct realtimeRunnerParameters {
    int tNum;
    struct GC_state *state;
};

void realtimeThreadInit(struct GC_state *state, pthread_t *, pthread_t *);
void *realtimeRunner(void* paramsPtr);
void realtimeThreadWaitForInit(void);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))
pointer FFI_getOpArgsResPtr (GC_state s);

int32_t GC_threadYield(GC_state s);
int32_t GC_myPriority(GC_state s);

#endif

#endif /* _REALTIME_THREAD_H_ */
