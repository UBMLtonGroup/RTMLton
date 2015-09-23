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

	if (p) {
		gct = (GC_thread)(p + offsetofThread (s));

		n = RTThread_findThreadAndQueue(gct, &pri);

		if (n) return pri;
	}

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

	if (p) {
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
	}
	else {
		fprintf(stderr, "warning null thread pointer passed to setThreadPriority\n");
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

	if (p) {
		gct = (GC_thread)(p + offsetofThread (s));

		LOCK(thread_queue_lock);
		n = RTThread_findThreadAndQueue(gct, &pri);
		UNLOCK(thread_queue_lock);

		if (n)
			n->runnable = TRUE;
		else
			return 0;
	}
	else {
		fprintf(stderr, "warning null thread pointer passed to setThreadRunnable\n");
	}

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

	if (DEBUG)
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
		if (DEBUG)
			fprintf(stderr, "add to head\n");
		thread_queue[priority].head = node;
		thread_queue[priority].tail = node;
	}
	else {
		if (DEBUG)
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
			malloc((MAXPRI+1) * sizeof(pthread_t));
	MYASSERT(long, realtimeThreads, !=, NULL);

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

}

__attribute__ ((noreturn))
void* realtimeRunner(void* paramsPtr) {
    struct realtimeRunnerParameters *params = paramsPtr;
	struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num(params->tNum);

    assert(params->tNum == PTHREAD_NUM);

	if (params->tNum <= 1) {
		while(1) sleep(10000); //pthread_exit(NULL);
	}

	while (!(state->callFromCHandlerThread != BOGUS_OBJPTR)) {
		if (DEBUG) fprintf(stderr, "%d] spin [callFromCHandlerThread == 1] %x ..\n", tNum, params->state);
	}

	fprintf(stderr, "%d] callFromCHandlerThread %x is ready\n", tNum, state->callFromCHandlerThread);

#if 1
	GC_thread curct = (GC_thread)(objptrToPointer(state->currentThread[0], state->heap.start) + offsetofThread (state));
	//GC_stack curstk = newStack(state, 128, FALSE);
	GC_stack curstk = (GC_stack)(objptrToPointer(curct->stack, state->heap.start));
	GC_thread tc = copyThread(state, curct, curstk->used);
	state->currentThread[PTHREAD_NUM] = pointerToObjptr((pointer)(tc - offsetofThread (state)), state->heap.start);
#else
	state->currentThread[PTHREAD_NUM] = newThread(state, 128);
#endif

    while (1) {
        if (DEBUG) {
        	fprintf(stderr, "%d] realtimeRunner running.\n", tNum);
        	fprintf(stderr, "%d] calling Parallel_run..\n", tNum);
        }
        Parallel_run();
        fprintf(stderr, "%d] back from Parallel_run (shouldnt happen)\n", tNum);
        exit(-1);
    }
}

pointer FFI_getOpArgsResPtr (GC_state s) {
  return s->ffiOpArgsResPtr[PTHREAD_NUM];
}
