#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

#define MAXPRI 3 /* 0 = main, 1 = GC, */

#if (defined (MLTON_GC_INTERNAL_FUNCS))
struct realtimeRunnerParameters {
    int tNum;
    struct GC_state *state;
};


typedef struct _TQNode TQNode;
struct _TQNode {
	GC_thread t;
	bool runnable;
	TQNode *next;
	TQNode *prev;
};

TQNode *RTThread_findThreadInQueue(GC_thread t, int32_t priority);
TQNode *RTThread_findThreadAndQueue(GC_thread t, int32_t *priority);

int RTThread_addThreadToQueue(GC_thread t, int32_t priority);
TQNode *RTThread_unlinkThreadFromQueue(GC_thread t, int32_t priority);

void realtimeThreadInit(struct GC_state *state, pthread_t *, pthread_t *);
void *realtimeRunner(void* paramsPtr);
void realtimeThreadWaitForInit(void);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))
pointer FFI_getOpArgsResPtr (GC_state s);

int32_t GC_setThreadPriority(GC_state s, pointer p, int32_t prio);
int32_t GC_getThreadPriority(GC_state s, pointer p);
int32_t GC_threadYield(GC_state s);
int32_t GC_setThreadRunnable(GC_state s, pointer p);
int32_t GC_displayThreadQueue(GC_state s, int32_t);
int32_t GC_myPriority(GC_state s);

#endif

#endif /* _REALTIME_THREAD_H_ */
