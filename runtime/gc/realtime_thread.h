#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

#define MAXPRI 10 /* 0 .. 10 */


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

TQNode *RTThread_findThread(GC_thread t);
TQNode *RTThread_findThreadInQueue(GC_thread t, int32_t priority);
int RTThread_addThreadToQueue(GC_thread t, int32_t priority);
TQNode *RTThread_unlinkThreadFromQueue(GC_thread t, int32_t priority);

void realtimeThreadInit(struct GC_state *state);
void *realtimeRunner(void* paramsPtr);
int allocate_pthread(struct GC_state *state, struct cont *cont);

/* Unique number for this thread */
int32_t Proc_processorNumber (GC_state s);
/* Is the current processor the primary processor? */
bool Proc_amPrimary (GC_state s);

/* Used to make sure all threads are properly initialized */
void RTThread_waitForInitialization (GC_state s);
void Proc_signalInitialization (GC_state s);
bool RTThread_isInitialized (GC_state s);

/* Synchronize all processors */
void Proc_beginCriticalSection (GC_state s);
void Proc_endCriticalSection (GC_state s);
bool Proc_threadInSection (__attribute__ ((unused)) GC_state s);
bool Proc_executingInSection (__attribute__ ((unused)) GC_state s);


#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))

int32_t GC_setThreadPriority(GC_state s, pointer p, int32_t prio);
int32_t GC_getThreadPriority(GC_state s, pointer p);
int32_t GC_threadYield(GC_state s);
int32_t GC_setThreadRunnable(GC_state s, pointer p);
int32_t GC_displayThreadQueue(GC_state s, int32_t);

#endif

#endif /* _REALTIME_THREAD_H_ */
