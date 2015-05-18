#include "realtime_thread.h"

volatile bool RTThreads_beginInit = FALSE;
volatile int32_t RTThreads_initialized= 0;
volatile int32_t Proc_criticalCount;
volatile int32_t Proc_criticalTicket;

pthread_mutex_t AllocatedThreadLock;

void RTThread_waitForInitialization (GC_state s) {
  int32_t unused;

  while (!RTThreads_beginInit) { }

  unused = __sync_fetch_and_add (&RTThreads_initialized, 1);

  while (!RTThread_isInitialized(s)) { }
  initProfiling (s);
}


bool RTThread_isInitialized (GC_state s) {
  return RTThreads_initialized == s->numRealtimeThreads;
}

void realtimeThreadInit(struct GC_state *state) {

        pthread_t *realtimeThreads =
                malloc(state->numRealtimeThreads * sizeof(pthread_t));
        assert(realtimeThreads != NULL);

        state->realtimeThreadConts =
                malloc(state->numRealtimeThreads * sizeof(struct cont));

	//Initializing the locks for each realtime thread created,
	state->realtimeThreadLocks= malloc(state->numRealtimeThreads *sizeof(pthread_mutex_t));
        int tNum;
        for (tNum = 2; tNum < state->numRealtimeThreads; tNum++) {
        	fprintf(stderr, "spawning thread %d\n", tNum);

            struct realtimeRunnerParameters* params =
                malloc(sizeof(struct realtimeRunnerParameters));
            params->tNum = tNum;
            params->state = state;

            state->realtimeThreadConts[tNum].nextChunk = NULL;

            if (pthread_create(&realtimeThreads[tNum], NULL, &realtimeRunner,
                        (void*)params)) {
                fprintf(stderr, "pthread_create failed: %s\n", strerror (errno));
                exit (1);
            }
        }

        state->realtimeThreads = realtimeThreads;

        state->realtimeThreadAllocated =
            malloc(state->numRealtimeThreads * sizeof(bool));
        for (tNum = 0; tNum < state->numRealtimeThreads; tNum++) {
            state->realtimeThreadAllocated[tNum] = false;
        }
}

void* realtimeRunner(void* paramsPtr) {

    struct realtimeRunnerParameters *params = paramsPtr;

    while (1) {
        // Trampoline
        int tNum = params->tNum;
        printf("\t%x] realtimeRunner[%d] running.\n", pthread_self(), tNum);

        // copy the cont struct to this local variable
        struct GC_state *state = params->state;
        printf("state = %x\n", state);

        // TODO lock lock[tNum]
	//Acquiring lock associated with pThread from GC state
	pthread_mutex_lock(&state->realtimeThreadLocks[tNum]);


        printf("Acquired thread lock\n");

        struct cont* realtimeThreadConts = state->realtimeThreadConts;
        printf("realtimeThreadConts = %x\n", realtimeThreadConts);

        printf("cont.nextChunk: if next line doesn't print, cont is null\n");
        struct cont cont = realtimeThreadConts[tNum];

        printf("cont.nextChunk = %x\n", cont.nextChunk);
        if (cont.nextChunk != NULL) {
            printf("\t%x] realtimeRunner trampolining.\n", pthread_self());
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();
            cont=(*(struct cont(*)(void))cont.nextChunk)();

            // copy local variable back to gcstate
            params->state->realtimeThreadConts[tNum] = cont;
	
        } else {
            printf("\t%x] realtimeRunner has nothing to trampoline.\n", pthread_self());
        }

        // TODO unlock lock[tNum]
	pthread_mutex_unlock(&state->realtimeThreadLocks[tNum]);
        printf("Released thread lock \n");
    }
    return NULL;
}

int allocate_pthread(struct GC_state *state, struct cont *cont) {

	//grab a lock for accessing the allocated threads structure

	pthread_mutex_lock(&AllocatedThreadLock);
	
	//Loop through allocate thread structure to find free spot
	for(int i=0;i<state->numRealtimeThreads;i++)
	{
		if(!state->realtimeThreadAllocated[i])
		  {	
			pthread_mutex_lock(&state->realtimeThreadLocks[i]);
			state->realtimeThreadConts[i] = *cont;
			pthread_mutex_unlock(&state->realtimeThreadLocks[i]);
			
			//Mark thread as allocated
			state->realtimeThreadAllocated[i]=true;
			pthread_mutex_unlock(&AllocatedThreadLock);
			return i;
		  }	
	}
	
	pthread_mutex_unlock(&AllocatedThreadLock);
	return -1;

}
