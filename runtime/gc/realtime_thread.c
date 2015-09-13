/* http://stackoverflow.com/questions/3649281/how-to-increase-thread-priority-in-pthreads */

#include "realtime_thread.h"

#define LOCK(X) { MYASSERT(int, pthread_mutex_lock(&X), ==, 0); }
#define UNLOCK(X) { MYASSERT(int, pthread_mutex_unlock(&X), ==, 0); }

static pthread_mutex_t thread_queue_lock;

static int RTThread_addThreadToQueue_nolock(GC_thread t, int32_t priority);

static struct _TQ {
	TQNode *head;
	TQNode *tail;
} thread_queue[MAXPRI];

volatile bool RTThreads_beginInit = FALSE;
volatile int32_t RTThreads_initialized= 0;
volatile int32_t Proc_criticalCount;
volatile int32_t Proc_criticalTicket;

pthread_mutex_t AllocatedThreadLock;

int GC_displayThreadQueue(__attribute__ ((unused)) GC_state s, __attribute__ ((unused)) int unused) {
	LOCK(thread_queue_lock);
	for(int i = 0 ; i < MAXPRI ; i++) {
		int count = 0;
		for(TQNode *n = thread_queue[i].head ; n != NULL ; n = n->next, count++);
		fprintf(stderr, "priority: %d num_threads: %d\n", i, count);
	}
	UNLOCK(thread_queue_lock);
	return 0;
}

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
			n->next = n->prev = NULL;
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
	TQNode *n;
	int pri = 0;

	if (DEBUG)
		fprintf(stderr, "GC_getThreadPriority("FMTPTR")\n", (uintptr_t)p);

	gct = (GC_thread)(p + offsetofThread (s));

	n = RTThread_findThreadAndQueue(gct, &pri);

	if (n) return pri;

	return -1;
}

/* TODO
 * lookup thread,
 * change prio in GC_thread struct,
 * move thread to new queue if nec'y
 * change the posix priority
 */
int32_t GC_setThreadPriority(GC_state s, pointer p, int32_t prio) {
	GC_thread gct;
	TQNode *n;
	int old_pri = 0;

	if (DEBUG)
		fprintf(stderr, "GC_setThreadPriority("FMTPTR", %d)\n", (uintptr_t)p, prio);

	gct = (GC_thread)(p + offsetofThread (s));

	LOCK(thread_queue_lock);

	n = RTThread_findThreadAndQueue(gct, &old_pri);

	if (n) {
		RTThread_unlinkThreadFromQueue(gct, old_pri);
		RTThread_addThreadToQueue_nolock(gct, prio);
	}

	UNLOCK(thread_queue_lock);

	if (DEBUG) {
		displayThread(s, gct, stderr);
		displayStack(s, (GC_stack)(gct->stack), stderr);
	}

	return prio;
}

int32_t GC_myPriority(__attribute__ ((unused)) GC_state s)
{
	return PTHREAD_NUM;
}

/* TODO
 * lookup thread,
 * change runnable to true,
 */
int32_t GC_setThreadRunnable(GC_state s, pointer p) {
	GC_thread gct;
	TQNode *n = NULL;
	int pri;

	if (DEBUG)
		fprintf(stderr, "GC_setThreadRunnable("FMTPTR")\n", (uintptr_t)p);

	gct = (GC_thread)(p + offsetofThread (s));

	LOCK(thread_queue_lock);
	n = RTThread_findThreadAndQueue(gct, &pri);
	UNLOCK(thread_queue_lock);

	if (n)
		n->runnable = TRUE;
	else
		return 0;
	return 1;
}

/* TODO
 * move thread to back of queue
 * pthread_yield
 */
int32_t GC_threadYield(__attribute__ ((unused)) GC_state s) {
	fprintf(stderr, "GC_threadYield()\n");
	return 0;
}

TQNode *RTThread_findThreadInQueue(GC_thread t, int32_t priority) {
	TQNode *n = NULL;
	for (n = thread_queue[priority].head ; n != NULL && n->t != t ; n = n->next);
	return n;
}

TQNode *RTThread_findThreadAndQueue(GC_thread t, int32_t *priority) {
	TQNode *n = NULL;
	int i;

	if (priority == NULL) return NULL;

	for (i = 0 ; i < MAXPRI ; i++) {
		for (n = thread_queue[i].head ; n != NULL ; n = n->next) {
			if (n->t == t) {
				*priority = i;
				i = MAXPRI;
				break;
			}
		}
	}

	return n;
}


TQNode *RTThread_unlinkThreadFromQueue(GC_thread t, int32_t priority) {
	TQNode *n = NULL;
	fprintf(stderr, "unlinkThreadFromQueue(%x, %d)", t, priority);

	for (n = thread_queue[priority].head ; n != NULL && n->t != t ; n = n->next);
	if (n) {
		if (n == thread_queue[priority].head && n == thread_queue[priority].tail) {
			thread_queue[priority].head = NULL;
			thread_queue[priority].tail = NULL;
		}
		else if (n == thread_queue[priority].head) {
			thread_queue[priority].head = n->next;
		}
		else if (n == thread_queue[priority].tail) {
			thread_queue[priority].tail->prev->next = NULL;
			thread_queue[priority].tail = n->prev;
		}
		else {
			n->prev->next = n->next;
		}

		n->prev = NULL;
		n->next = NULL;
	}
	return n;
}

static int RTThread_addThreadToQueue_nolock(GC_thread t, int32_t priority) {
	TQNode *node;

	if (DEBUG)
		fprintf(stderr, "addThreadToQueue(%x, pri=%d)\n", t, priority);

    // priority 1 is reserved to the GC
	if (t == NULL || priority < 0 || priority == 1 || priority > MAXPRI) return -1;

	node = make_tqnode(t);

	if (thread_queue[priority].head == NULL) {
		fprintf(stderr, "add to head\n");
		thread_queue[priority].head = node;
		thread_queue[priority].tail = node;
	}
	else {
		fprintf(stderr, "add to tail\n");
		TQNode *_t = thread_queue[priority].tail;
		thread_queue[priority].tail = node;
		_t->next = node;
		node->prev = _t;
	}

	return 0;
}

int RTThread_addThreadToQueue(GC_thread t, int32_t priority) {
	int rv = -1;
	LOCK(thread_queue_lock);
	rv = RTThread_addThreadToQueue_nolock(t, priority);
	UNLOCK(thread_queue_lock);
	return rv;
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

	//Initializing the locks for each realtime thread created,
	state->realtimeThreadLocks= malloc(MAXPRI * sizeof(pthread_mutex_t));

	int tNum;
	for (tNum = 0; tNum < MAXPRI; tNum++) {
		if (DEBUG)
			fprintf(stderr, "spawning thread %d\n", tNum);

		struct realtimeRunnerParameters* params =
			malloc(sizeof(struct realtimeRunnerParameters));

		params->tNum = tNum;
		params->state = state;

		if (pthread_create(&realtimeThreads[tNum], NULL, &realtimeRunner, (void*)params)) {
			fprintf(stderr, "pthread_create failed: %s\n", strerror (errno));
			exit (-1);
		}
	}

	state->realtimeThreads = realtimeThreads;

	state->realtimeThreadAllocated =
		malloc(MAXPRI * sizeof(bool));

	for (tNum = 0; tNum < MAXPRI; tNum++) {
		state->realtimeThreadAllocated[tNum] = false;
	}
}

__attribute__ ((noreturn))
void* realtimeRunner(void* paramsPtr) {

    struct realtimeRunnerParameters *params = paramsPtr;

    set_pthread_num(params->tNum);

    assert(params->tNum == PTHREAD_NUM);

    while (1) {
        // Trampoline
        int tNum = params->tNum;
        if (DEBUG)
        	printf("%lx] realtimeRunner[%d] running.\n", pthread_self(), tNum);

	sleep(1); /* testing.. slow things down so output is readable */

		if (tNum <= 1) continue;

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
    		struct GC_state *state = params->state;
			pointer thrp = (pointer)(node->t) - offsetofThread (state);
			objptr op = pointerToObjptr(thrp, state->heap.start);
        	struct cont cont;

        	if (DEBUG)
        		printf("%lx] pri %d has work to do\n", pthread_self(), PTHREAD_NUM);

        	/* run it, etc; steps 4-7
        	 *     node->t->stack
        	 *
        	 */

    		displayStack(state, (GC_stack)node->t->stack, stderr);

    		//GC_setSavedThread (state, thrp);
			switchToThread (state, op);

			nextFun = *(uintptr_t*)(state->stackTop[PTHREAD_NUM] - GC_RETURNADDRESS_SIZE);
			cont.nextChunk = nextChunks[nextFun];


			while(1){
				printf("%lx] pri %d trampolining\n", pthread_self(), PTHREAD_NUM);fflush(stdout);
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				printf("after first cont...\n");fflush(stdout);
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				cont=(*(struct cont(*)(void))cont.nextChunk)();
				
				// copy local variable back to gcstate
			}
        }
        else {
        	if (DEBUG)
        		printf("%lx] pri %d has nothing to do\n", pthread_self(), tNum);
		}
    }
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
