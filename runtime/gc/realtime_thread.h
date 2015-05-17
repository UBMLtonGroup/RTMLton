#ifndef _REALTIME_THREAD_H_
#define _REALTIME_THREAD_H_

#if (defined (MLTON_GC_INTERNAL_FUNCS))
struct realtimeRunnerParameters {
    int tNum;
    struct GC_state *state;
};

void realtimeThreadInit(struct GC_state *state);
void realtimeRunner(struct realtimeRunnerParameters* params);
void allocate_pthread(struct GC_state *state, struct cont *cont);

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
#endif /* _REALTIME_THREAD_H_ */
