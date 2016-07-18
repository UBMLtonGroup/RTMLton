/* http://stackoverflow.com/questions/3649281/how-to-increase-thread-priority-in-pthreads */

#include "realtime_thread.h"

#define LOCK(X) { MYASSERT(int, pthread_mutex_lock(&X), ==, 0); }
#define UNLOCK(X) { MYASSERT(int, pthread_mutex_unlock(&X), ==, 0); }

static pthread_mutex_t thread_queue_lock;
static volatile int initialized = 0;

static int RTThread_addThreadToQueue_nolock(GC_thread t, int32_t priority);

static struct _TQ {
	TQNode *head;
	TQNode *tail;
} thread_queue[MAXPRI];

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

int32_t GC_threadYield(__attribute__ ((unused)) GC_state s) {
	fprintf(stderr, "GC_threadYield()\n");
	sched_yield();
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


void realtimeThreadWaitForInit(void)
{
	while (initialized < MAXPRI) {
		fprintf(stderr, "spin [thread boot: %d out of %d]..\n", initialized, MAXPRI);
	}
}

void realtimeThreadInit(struct GC_state *state, pthread_t *main, pthread_t *gc) {
	int rv = 0;

	rv = pthread_mutex_init(&thread_queue_lock, NULL);
	MYASSERT(int, rv, ==, 0);
	memset(&thread_queue, 0, sizeof(struct _TQ)*(MAXPRI));

	state->realtimeThreads[0] = main;
	state->realtimeThreads[1] = gc;
	initialized = 2;
	// 0 = running, 1 = paused, 2 = not-ready
	state->threadPaused[0] = 0; // main is implicitly already running
	state->threadPaused[1] = 0; // GC we can set to running but is moot because we will never pause it

	int tNum;
	for (tNum = 2; tNum < MAXPRI; tNum++) {
		if (DEBUG)
			fprintf(stderr, "spawning thread %d\n", tNum);

		struct realtimeRunnerParameters* params =
			malloc(sizeof(struct realtimeRunnerParameters));

		params->tNum = tNum;
		params->state = state;

		pthread_t *pt = malloc(sizeof(pthread_t));
		memset(pt, 0, sizeof(pthread_t));

		if (pthread_create(pt, NULL, &realtimeRunner, (void*)params)) {
			fprintf(stderr, "pthread_create failed: %s\n", strerror (errno));
			exit (-1);
		}
		else {
			state->realtimeThreads[tNum] = pt;
			state->threadPaused[tNum]=0; // thread not paused, spinning.
			initialized++;
		}
	}
	state->isRealTimeThreadInitialized = TRUE;

}

__attribute__ ((noreturn))
void* realtimeRunner(void* paramsPtr) {
    struct realtimeRunnerParameters *params = paramsPtr;
	struct GC_state *state = params->state;
    int tNum = params->tNum;

    set_pthread_num(params->tNum);

    assert(params->tNum == PTHREAD_NUM);

	while (!(state->callFromCHandlerThread != BOGUS_OBJPTR)) {
		if (DEBUG) {
			fprintf(stderr, "%d] spin [callFromCHandlerThread boot] ..\n", tNum);
		
		}

		/*If any thread has already requested a GC, the current thread can reach a GC safe point by calling perform GC itself,
		 * since it is only spinning and waiting to be linked to an SML computation. Calling the performGC function
		 * allows the current thread to systematically pause itself, although it doesnt need to GC and can be paused at any instant. */
		if(state->GCRequested)
		{
			
			fprintf(stderr, "%d] Other thread requested GC. Moving to safe point. \n", tNum);
			//call performGC with the state of prev executing thread as current thread has no computation
			performGC(state,state->oldGenBytesRequested,state->nurseryBytesRequested,state->forceMajor,state->mayResize); 
		}
		ssleep(1, 0);
	}

	fprintf(stderr, "%d] callFromCHandlerThread %x is ready\n", tNum, state->callFromCHandlerThread);

#if 1
	/* state->currentThread objptr
	   curct->stack         objptr

	   ref: https://github.com/UBMLtonGroup/MLton/blob/master/runtime/gc/switch-thread.c#L14-L16

	   given an objptr, to get GC_thread 

		thread = (GC_thread)(objptrToPointer (op, s->heap.start)
                         + offsetofThread (s));

	   given a pointer, to get objptr 

		stack = (GC_stack)objptrToPointer (thread->stack, s->heap.start))

	   from is a GC_thread in the following.
	   (objptr) s->savedThread = pointerToObjptr((pointer)from - offsetofThread (s), s->heap.start);

	   pointer GC_copyThread (GC_state s, pointer p) 

	 */

	GC_thread curct = (GC_thread)(objptrToPointer(state->currentThread[0], state->heap.start)
				+ offsetofThread (state));
	GC_stack curstk = (GC_stack)objptrToPointer(curct->stack, state->heap.start);

	/* GC_thread copyThread (GC_state s, GC_thread from, size_t used) */

	GC_thread tc = copyThread(state, curct, curstk->used);

	state->currentThread[PTHREAD_NUM] = pointerToObjptr((pointer)(tc - offsetofThread (state)), state->heap.start);
#else
	/* cant do this bc it requires state->currentThread to already be set */
	state->currentThread[PTHREAD_NUM] = pointerToObjptr(GC_copyThread (state, objptrToPointer(
		state->currentThread[0], state->heap.start)), state->heap.start); 
#endif

	state->threadPaused[params->tNum] = 0;

	state->isRealTimeThreadRunning =TRUE;

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
