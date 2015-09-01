/* http://stackoverflow.com/questions/3649281/how-to-increase-thread-priority-in-pthreads */

#include "realtime_thread.h"

#define LOCK(X) { MYASSERT(int, pthread_mutex_lock(&X), ==, 0); }
#define UNLOCK(X) { MYASSERT(int, pthread_mutex_unlock(&X), ==, 0); }

#define MAXPRI 10 /* 0 .. 10 */

static pthread_mutex_t thread_queue_lock;

static struct _TQ {
	TQNode *head;
	TQNode *tail;
} thread_queue[MAXPRI];

volatile bool RTThreads_beginInit = FALSE;
volatile int32_t RTThreads_initialized= 0;
volatile int32_t Proc_criticalCount;
volatile int32_t Proc_criticalTicket;

pthread_mutex_t AllocatedThreadLock;

static TQNode* find_runnable(struct _TQ queue) {
	TQNode *n = queue.head;
	while(n != NULL && n->runnable == FALSE)
		n = n->next;
	return n;
}

static TQNode* make_tqnode(GC_thread t) {
	TQNode *n = NULL;
	if (t != NULL) {
		n = malloc(sizeof(TQNode));
		if (n != NULL) {
			n->t = t;
			n->runnable = FALSE;
			n->next = NULL;
		}
		else {
			fprintf(stderr, "make_tqnode: out of memory\n");
			exit(-1);
		}
	}
	return n;
}

int32_t GC_getThreadPriority(GC_state s, pointer p) {
	GC_thread gct;
	fprintf(stderr, "GC_getThreadPriority("FMTPTR")\n", (uintptr_t)p);

	gct = (GC_thread)(p + offsetofThread (s));

	return gct->prio;
}

/* TODO
 * lookup thread,
 * change prio in GC_thread struct,
 * move thread to new queue if nec'y
 * change the posix priority
 */
int32_t GC_setThreadPriority(GC_state s, pointer p, int32_t prio) {
	GC_thread gct;

	fprintf(stderr, "GC_setThreadPriority("FMTPTR", %d)\n", (uintptr_t)p, prio);
	gct = (GC_thread)(p + offsetofThread (s));
	gct->prio = prio;

	displayThread(s, gct, stderr);
	displayStack(s, (GC_stack)(gct->stack), stderr);

	return prio;
}

/* TODO
 * move thread to back of queue
 * pthread_yield
 */
int32_t GC_threadYield(__attribute__ ((unused)) GC_state s) {
	fprintf(stderr, "GC_threadYield()\n");
	return 0;
}

TQNode *RTThread_findThreadInQueue(TQNode *head, GC_thread t) {
	TQNode *n = NULL;
	for (n = head ; n != NULL && n->t != t ; n = n->next);
	return n;
}

int RTThread_addThreadToQueue(GC_thread t, int priority) {
	TQNode *node;

    fprintf(stderr, "addThreadToQueue(pri=%d)\n", priority);

    // priority 1 is reserved to the GC
	if (t == NULL || priority < 0 || priority == 1 || priority > MAXPRI) return -1;

	LOCK(thread_queue_lock);
	node = make_tqnode(t);
	if (thread_queue[priority].head == NULL) {
		thread_queue[priority].head = thread_queue[priority].tail = node;
	}
	else {
		thread_queue[priority].tail->next = node;
	}

	UNLOCK(thread_queue_lock);

	return 0;
}

void RTThread_waitForInitialization (GC_state s) {
	__attribute__ ((unused)) int32_t unused;

  while (!RTThreads_beginInit) { }

  unused = __sync_fetch_and_add (&RTThreads_initialized, 1);

  while (!RTThread_isInitialized(s)) { }
  initProfiling (s);
}


bool RTThread_isInitialized (__attribute__ ((unused)) GC_state s) {
  return RTThreads_initialized == MAXPRI;
}

void realtimeThreadInit(struct GC_state *state) {
	int rv = 0;

	rv = pthread_mutex_init(&thread_queue_lock, NULL);
	MYASSERT(int, rv, ==, 0);
	memset(&thread_queue, 0, sizeof(struct _TQ)*(MAXPRI));

	pthread_t *realtimeThreads =
			malloc(MAXPRI * sizeof(pthread_t));
	MYASSERT(long, realtimeThreads, !=, NULL);

	state->realtimeThreadConts =
			malloc(MAXPRI * sizeof(struct cont));

	//Initializing the locks for each realtime thread created,
	state->realtimeThreadLocks= malloc(MAXPRI * sizeof(pthread_mutex_t));

	int tNum;
	for (tNum = 0; tNum < MAXPRI; tNum++) {
		fprintf(stderr, "spawning thread %d\n", tNum);

		struct realtimeRunnerParameters* params =
			malloc(sizeof(struct realtimeRunnerParameters));

		params->tNum = tNum;
		params->state = state;

		state->realtimeThreadConts[tNum].nextChunk = NULL;

		set_pthread_num(0); // "main" is thread0, GC is thread1

		if (tNum > 1) {
			if (pthread_create(&realtimeThreads[tNum], NULL, &realtimeRunner, (void*)params)) {
				fprintf(stderr, "pthread_create failed: %s\n", strerror (errno));
				exit (1);
			}
		}
	}

	state->realtimeThreads = realtimeThreads;

	state->realtimeThreadAllocated =
		malloc(MAXPRI * sizeof(bool));

	for (tNum = 0; tNum < MAXPRI; tNum++) {
		state->realtimeThreadAllocated[tNum] = false;
	}
}

void* realtimeRunner(void* paramsPtr) {

    struct realtimeRunnerParameters *params = paramsPtr;

    set_pthread_num(params->tNum);

    while (1) {
        // Trampoline
        int tNum = params->tNum;
        printf("%lx] realtimeRunner[%d] running.\n", pthread_self(), tNum);

	sleep(1); /* testing.. slow things down so output is readable */

		/* 1. lock the queue
		 * 2. find a runnable thread
		 * 3. unlock the queue
		 * 4. run it until it completes or yields
		 * 5. lock the queue
		 * 6. move it to the back of the queue
		 * 7. unlock the queue
		 * 8. repeat
		 */
        LOCK(thread_queue_lock);
        TQNode *node = find_runnable(thread_queue[params->tNum]);
		UNLOCK(thread_queue_lock);

        if (node != NULL) {
        	printf("%lx] pri %d has work to do\n", pthread_self(), tNum);

        	/* run it, etc; steps 4-7
        	 *     node->t->stack
        	 *
        	 */

			// copy the cont struct to this local variable
			struct GC_state *state = params->state;

			// TODO lock lock[tNum]
			//Acquiring lock associated with pThread from GC state
			pthread_mutex_lock(&state->realtimeThreadLocks[tNum]);

			struct cont* realtimeThreadConts = state->realtimeThreadConts;

			struct cont cont = realtimeThreadConts[tNum];

			if (cont.nextChunk != NULL) {
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

			}

			// TODO unlock lock[tNum]
			pthread_mutex_unlock(&state->realtimeThreadLocks[tNum]);
        }
        else {
			printf("%lx] pri %d has nothing to do\n", pthread_self(), tNum);
		}
    }
    return NULL;
}

int allocate_pthread(struct GC_state *state, struct cont *cont) {

	//grab a lock for accessing the allocated threads structure

	pthread_mutex_lock(&AllocatedThreadLock);
	
	//Loop through allocate thread structure to find free spot
	for(int i = 0 ; i < MAXPRI ; i++)
	{
		if(!state->realtimeThreadAllocated[i])
		  {	
			pthread_mutex_lock(&state->realtimeThreadLocks[i]);
			state->realtimeThreadConts[i] = *cont;
			pthread_mutex_unlock(&state->realtimeThreadLocks[i]);
			
			//Mark thread as allocated
			state->realtimeThreadAllocated[i] = true;
			pthread_mutex_unlock(&AllocatedThreadLock);
			return i;
		  }	
	}
	
	pthread_mutex_unlock(&AllocatedThreadLock);
	return -1;

}
