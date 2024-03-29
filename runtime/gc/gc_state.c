#include "../gc.h"

/* Copyright (C) 2009,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */


//#define BROADCAST_RT_THREADS IFED(pthread_cond_broadcast(&s->rtThreads_cond))
#define LOCK_RT_TH IFED(pthread_mutex_lock(&s->rtThreads_lock))
#define UNLOCK_RT_TH IFED(pthread_mutex_unlock(&s->rtThreads_lock))

void displayGCState(GC_state s, FILE *stream) {

	if (DEBUG_DETAILED) {
		if (PTHREAD_NUM == 1) {
			fprintf(stream, "%d] Cannot display GC_state in GC thread\n", PTHREAD_NUM);
			return;
		}

		fprintf(stream, "%d] GC state\n", PTHREAD_NUM);
		fprintf(stream, "%d] \tcurrentThread = "FMTOBJPTR"\n", PTHREAD_NUM, s->currentThread[PTHREAD_NUM]);
		displayThread(s, (GC_thread) (objptrToPointer(s->currentThread[PTHREAD_NUM], s->umheap.start)
									  + offsetofThread(s)),
					  stream);
		fprintf(stream, "%d] \t Global heap\n", PTHREAD_NUM);
		displayHeap(s, &s->globalHeap,
					stream);
		fprintf(stream,
				"%d] \tlimit = "FMTPTR"\n"
				"\tcurrentFrame = "FMTPTR"\n",
				PTHREAD_NUM,
				(uintptr_t) s->limit,
				(uintptr_t) s->currentFrame[PTHREAD_NUM]
		);
	}
}

/* in the previous mlton stack implementation, stacktop
 * pointed to the first word of the top most frame, and a
 * stack write (eg S(x)=y) was equivalent to *(stacktop+x) = y
 *
 * so used was stacktop - stackbottom
 *
 * in the stacklet implementation, stacktop is the currentframe (chunk)
 * and stackbottom is the first frame (chunk) and so "used"
 * doesnt have a trivial arithmetic correlation anymore.
 * firstframe and currentframe are kept in the thread structure.
 */
size_t sizeofGCStateCurrentStackUsed(GC_state s) {
	die(RED("***ERR*** sizeofGCStateCurrentStackUsed needs to be removed?\n"));
	return (0);
}

/* in RTMLton, the amount of 'stack used' corresponds to the number of
 * chunks in use in the current stack.
 */
size_t sizeofGCStateCurrentUMStackUsed(GC_state s) {
	return (0);
}

void setGCStateCurrentThreadAndStack(GC_state s) {
	GC_thread thread;
	//C_stack stack;

	thread = getThreadCurrent(s);

	objptr currentFrame = thread->currentFrame;
	objptr firstFrame = thread->firstFrame;

	s->exnStack[PTHREAD_NUM] = thread->exnStack;
	s->currentFrame[PTHREAD_NUM] = currentFrame;
	s->stackDepth[PTHREAD_NUM] = thread->stackDepth;

	if (DEBUG_DETAILED)
		fprintf(stderr, "%d] "
						GREEN("setGCStateCurrentThreadAndStack")
						" currentFrame "FMTPTR" stackBottom "FMTPTR" exnStack "FMTPTR"\n",
				PTHREAD_NUM,
				(uintptr_t) s->currentFrame[PTHREAD_NUM],
				(uintptr_t) firstFrame,
				(uintptr_t) s->exnStack[PTHREAD_NUM]);

}

__attribute__ ((unused)) void setGCStateCurrentHeap(GC_state s,
													size_t oldGenBytesRequested,
													size_t nurseryBytesRequested) {

//old heap nuked. wont need to set these items
	die("Trying to set current heap \n");

#if 0
	GC_heap h;
	pointer nursery;
	size_t nurserySize;
	pointer genNursery;
	size_t genNurserySize;

	if (DEBUG_DETAILED)
	  fprintf (stderr, "setGCStateCurrentHeap(%s, %s)\n",
			   uintmaxToCommaString(oldGenBytesRequested),
			   uintmaxToCommaString(nurseryBytesRequested));
	h = &s->heap;
	assert (isFrontierAligned (s, h->start + h->oldGenSize + oldGenBytesRequested));
	s->limitPlusSlop = h->start + h->size;
	s->limit = s->limitPlusSlop - GC_HEAP_LIMIT_SLOP;
	nurserySize = h->size - (h->oldGenSize + oldGenBytesRequested);
	assert (isFrontierAligned (s, s->limitPlusSlop - nurserySize));
	nursery = s->limitPlusSlop - nurserySize;
	genNursery = alignFrontier (s, s->limitPlusSlop - (nurserySize / 2));
	genNurserySize = (size_t)(s->limitPlusSlop - genNursery);
	if (/* The mutator marks cards. */
		s->mutatorMarksCards
		/* There is enough space in the generational nursery. */
		and (nurseryBytesRequested <= genNurserySize)
		/* The nursery is large enough to be worth it. */
		and (((float)(h->size - s->lastMajorStatistics.bytesLive)
			  / (float)nurserySize)
			 <= s->controls.ratios.nursery)
		and /* There is a reason to use generational GC. */
		(
		 /* We must use it for debugging purposes. */
		 FORCE_GENERATIONAL
		 /* We just did a mark compact, so it will be advantageous to to use it. */
		 or (s->lastMajorStatistics.kind == GC_MARK_COMPACT)
		 /* The live ratio is low enough to make it worthwhile. */
		 or ((float)h->size / (float)s->lastMajorStatistics.bytesLive
			 <= (h->withMapsSize < s->sysvals.ram
				 ? s->controls.ratios.copyGenerational
				 : s->controls.ratios.markCompactGenerational))
		 )) {
	  s->canMinor = TRUE;
	  nursery = genNursery;
	  nurserySize = genNurserySize;
	  clearCardMap (s);
	} else {
	  unless (nurseryBytesRequested <= nurserySize)
		die ("Out of memory.  Insufficient space in nursery.");
	  s->canMinor = FALSE;
	}
	assert (nurseryBytesRequested <= nurserySize);
	s->heap.nursery = nursery;
	s->frontier = nursery;
	assert (nurseryBytesRequested <= (size_t)(s->limitPlusSlop - s->frontier));
	assert (isFrontierAligned (s, s->heap.nursery));
	assert (hasHeapBytesFree (s, oldGenBytesRequested, nurseryBytesRequested));
#endif
}

bool GC_getAmOriginal(GC_state s) {
	return s->amOriginal;
}

void GC_setAmOriginal(GC_state s, bool b) {
	s->amOriginal = b;
}

void GC_setControlsMessages(GC_state s, bool b) {
	s->controls.messages = b;
}

void GC_setControlsSummary(GC_state s, bool b) {
	s->controls.summary = b;
}

void GC_setControlsRusageMeasureGC(GC_state s, bool b) {
	s->controls.rusageMeasureGC = b;
}


uintmax_t GC_getCumulativeStatisticsBytesAllocated(GC_state s) {
	return s->cGCStats.numChunksAllocated;
}

uintmax_t GC_getCumulativeStatisticsNumSweeps(GC_state s) {
	return s->cGCStats.numSweeps;
}

uintmax_t GC_getCumulativeStatisticsTotalGCTime(GC_state s) {
	return s->cGCStats.totalGCTime;
}

uintmax_t GC_getCumulativeStatisticsNumChunksFreed(GC_state s) {
	return s->cGCStats.numChunksFreed;
}

uintmax_t GC_getCumulativeStatisticsMaxMutatorPauseTime(GC_state s) {
	return s->cGCStats.maxMutatorPauseTime;
}

void GC_setHashConsDuringGC(GC_state s, bool b) {
	s->hashConsDuringGC = b;
}

size_t GC_getLastMajorStatisticsBytesLive(GC_state s) {
	return s->lastMajorStatistics.bytesLive;
}


pointer GC_getCallFromCHandlerThread(GC_state s) {
	pointer p = objptrToPointer(s->callFromCHandlerThread[PTHREAD_NUM], s->umheap.start);
	return p;
}

void GC_setCallFromCHandlerThread(GC_state s, pointer p) {
	objptr op = pointerToObjptr(p, s->umheap.start);
	s->callFromCHandlerThread[PTHREAD_NUM] = op;
	if (DEBUG_THREADS)
		fprintf(stderr, "%d] "PURPLE("call handler set")": "FMTPTR"\n", PTHREAD_NUM, (uintptr_t)op);
	GC_copyCurrentThread(s, false);
	//LOCK_RT_TH;
	//BROADCAST_RT_THREADS;
	//UNLOCK_RT_TH;
}

pointer GC_getCurrentThread(GC_state s) {
	pointer p = objptrToPointer(s->currentThread[PTHREAD_NUM], s->umheap.start);
	return p;
}

pointer GC_getSavedThread(GC_state s) {
	pointer p;
	assert(s->savedThread[PTHREAD_NUM] != BOGUS_OBJPTR);
	p = objptrToPointer(s->savedThread[PTHREAD_NUM], s->umheap.start);
    if (DEBUG_THREADS) {
        fprintf(stderr, "%d] %s "RED("get savedThread")" returns="FMTPTR" (and resets saved thread to "FMTPTR")\n",
			PTHREAD_NUM, __FUNCTION__, (uintptr_t)p, (uintptr_t)BOGUS_OBJPTR);
	}
	s->savedThread[PTHREAD_NUM] = BOGUS_OBJPTR;
	return p;
}

void GC_setSavedThread(GC_state s, pointer p) {
	objptr op;

	assert(s->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);
	op = pointerToObjptr(p, s->umheap.start);
	s->savedThread[PTHREAD_NUM] = op;
    if(DEBUG_THREADS)
        fprintf(stderr, "%d] %s "RED("setting savedThread")" to "FMTPTR"\n",
			PTHREAD_NUM, __FUNCTION__, (uintptr_t)s->savedThread[PTHREAD_NUM]);
}


void GC_setSignalHandlerThread(GC_state s, pointer p) {
	objptr op = pointerToObjptr(p, s->umheap.start);
	s->signalHandlerThread[PTHREAD_NUM] = op;
	if (DEBUG_THREADS)
		fprintf(stderr, "%d] "PURPLE("signal handler set")": "FMTPTR"\n", PTHREAD_NUM, (uintptr_t)op);
}

struct rusage *GC_getRusageGCAddr(GC_state s) {
	return &(s->cumulativeStatistics.ru_gc);
}

sigset_t *GC_getSignalsHandledAddr(GC_state s) {
	return &(s->signalsInfo.signalsHandled);
}

sigset_t *GC_getSignalsPendingAddr(GC_state s) {
	return &(s->signalsInfo.signalsPending);
}

void GC_setGCSignalHandled(GC_state s, bool b) {
	s->signalsInfo.gcSignalHandled = b;
}

bool GC_getGCSignalPending(GC_state s) {
	return (s->signalsInfo.gcSignalPending);
}

void GC_setGCSignalPending(GC_state s, bool b) {
	s->signalsInfo.gcSignalPending = b;
}

void push(GC_state s, int n) {
	//TODO worry about concurrent access to s->gcCallSeq
	int cnt = 0;
	while (cnt < MAXPRI) {
		if (s->gcCallSeq[cnt] == -1) {
			s->gcCallSeq[cnt] = n;
			return;
		}
		cnt++;
	}
}

int pop(GC_state s) {
	int cnt = MAXPRI - 1;
	while (cnt >= 0) {
		if (s->gcCallSeq[cnt] != -1) {
			int tmp = s->gcCallSeq[cnt];
			s->gcCallSeq[cnt] = -1;
			return tmp;
		}
		cnt--;
	}
	return -1;
}
