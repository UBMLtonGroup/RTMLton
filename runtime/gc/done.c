#include "../gc.h"

/* Copyright (C) 2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)
#define LOCK_FL IFED(pthread_mutex_lock(&s->fl_lock))
#define UNLOCK_FL IFED(pthread_mutex_unlock(&s->fl_lock))

#define BLOCK IFED(pthread_cond_wait(&s->fl_empty_cond,&s->fl_lock))

#if 0

static void displayCol(FILE *out, size_t width, const char *s) {
	size_t extra;
	size_t i;
	size_t len;

	len = strlen(s);
	if (len < width) {
		extra = width - len;
		for (i = 0; i < extra; i++)
			fprintf(out, " ");
	}
	fprintf(out, "%s\t", s);
}

static void displayCollectionStats(FILE *out, const char *name, struct rusage *ru,
								   uintmax_t num, uintmax_t bytes) {
	uintmax_t ms;

	ms = rusageTime(ru);
	fprintf(out, "%s", name);
	displayCol(out, 7, uintmaxToCommaString(ms));
	displayCol(out, 7, uintmaxToCommaString(num));
	displayCol(out, 15, uintmaxToCommaString(bytes));
	displayCol(out, 15,
			   (ms > 0)
			   ? uintmaxToCommaString((uintmax_t)(1000.0 * (float) bytes / (float) ms))
			   : "-");
	fprintf(out, "\n");
}
#endif

static void displayChunkedGCStats(GC_state s, FILE *out) {
	fprintf(out, "-------------\t-------\t-------\t---------------\t---------------\n");
	fprintf(out, "GC Statistics\n");
	fprintf(out, "-------------\t-------\t-------\t---------------\t---------------\n");
	fprintf(out, "Total chunks created = %d \n", (int) s->maxChunksAvailable);
	fprintf(out, "Number of Chunks allocated = %s\n", uintmaxToCommaString(s->cGCStats.numChunksAllocated));
	fprintf(out, "Number of Chunks Freed = %s\n", uintmaxToCommaString(s->cGCStats.numChunksFreed));
	fprintf(out, "Number of GC Sweeps = %s\n", uintmaxToCommaString(s->cGCStats.numSweeps));
	fprintf(out, "Total GC Time = %s\n", uintmaxToCommaString(s->cGCStats.totalGCTime));
	fprintf(out, "Mutator was blocked %d times\n", s->blocked);
	fprintf(out, "Max mutator pause time = %s\n", uintmaxToCommaString(s->cGCStats.maxMutatorPauseTime));
	fprintf(out, "Allocted by RT thread = %s\n", uintmaxToCommaString(s->allocedByRT));
	fprintf(out, "Stack Shrinks = %s\n", uintmaxToCommaString(s->cumulativeStatistics.stackShrinks));
	fprintf(out, "Stack Grows = %s\n", uintmaxToCommaString(s->cumulativeStatistics.stackGrows));
	fprintf(out, "Stack Forced Grows = %s\n", uintmaxToCommaString(s->cumulativeStatistics.forcedStackGrows));
}

void RTGC_done(GC_state s) {
	FILE *out;
	out = stderr;

    while(PTHREAD_NUM != 2 && s->useRTThreads)
    {
	//fprintf(stderr, "%d] Main thread spinning\n", PTHREAD_NUM);
        sched_yield();
    }

	/*If GC is running wait till its done*/
	if (s->isGCRunning) {
		LOCK_FL;
		if (DEBUG_RTGC)
			fprintf(stderr, "%d] Main thread blocking until GC is done\n", PTHREAD_NUM);

		BLOCK;

		if (DEBUG_RTGC)
			fprintf(stderr, "%d] Main thread Unblocked by GC\n", PTHREAD_NUM);
		UNLOCK_FL;
	}

	free(s->worklist);

	if (DISPLAY_GC_STATS)
		displayChunkedGCStats(s, out);
}
__attribute__((noreturn))
void GC_done(GC_state s) {

	die("GC_done shouldnt be called");
}
