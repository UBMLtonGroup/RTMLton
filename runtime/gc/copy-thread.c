/* Copyright (C) 2011-2012 Matthew Fluet.
 * Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#include "../gc.h"

GC_thread copyThread(GC_state s, GC_thread from, size_t used) {
	GC_thread to;

	/* newThread may do a GC, which invalidates from.
	 * Hence we need to stash from someplace that the GC can find it.
	 */

	if (s->savedThread[PTHREAD_NUM] != BOGUS_OBJPTR) {
		fprintf(stderr, PURPLE("%d] savedThread "FMTPTR" currentThread "FMTPTR" \n"),
		  PTHREAD_NUM,
		  s->savedThread[PTHREAD_NUM],
		  s->currentThread[PTHREAD_NUM]);
	}

	assert (s->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);

	s->savedThread[PTHREAD_NUM] = pointerToObjptr((pointer) from - offsetofThread(s), s->umheap.start);
	if (DEBUG_THREADS)
		fprintf(stderr, "%d]    %s "RED("setting savedThread")" to "FMTPTR"\n",
			PTHREAD_NUM, __FUNCTION__, s->savedThread[PTHREAD_NUM]);

	if (DEBUG_THREADS)
		fprintf(stderr, GREEN("%d]    copyThread from="FMTPTR" to="YELLOW("not-available-yet")"\n"
									  ), PTHREAD_NUM, (uintptr_t) from);
	to = newThread(s, used);

	if (DEBUG_THREADS)
		fprintf(stderr, GREEN("%d]    copyThread from="FMTPTR" to="
									  FMTPTR
									  "\n"), PTHREAD_NUM, (uintptr_t) from, (uintptr_t) to);

	from = (GC_thread) (objptrToPointer(s->savedThread[PTHREAD_NUM], s->umheap.start) + offsetofThread(s));
	s->savedThread[PTHREAD_NUM] = BOGUS_OBJPTR;
	fprintf(stderr, "%d]   %s "RED("setting savedThread")" to "FMTPTR"\n",
			PTHREAD_NUM, __FUNCTION__, s->savedThread[PTHREAD_NUM]);

	to->bytesNeeded = from->bytesNeeded; // TODO what does this do in stacklets?
	//to->exnStack = from->exnStack; // this will be adjusted in um_copyStack below

	um_copyStack(s, from, to); // note we pass in GC_threads not the actual stacklets

	return to;
}

void GC_copyCurrentThread(GC_state s, bool b) {
	GC_thread fromThread;
	GC_thread toThread;

	enter(s);
	fromThread = (GC_thread) (objptrToPointer(s->currentThread[PTHREAD_NUM], s->umheap.start)
							  + offsetofThread(s));
	fromThread->currentFrame = s->currentFrame[PTHREAD_NUM];

	if (DEBUG_THREADS)
		fprintf(stderr, "%d] GC_copyCurrentThread from="FMTPTR" to="YELLOW("not-available-yet")"\n",
			PTHREAD_NUM, (uintptr_t) fromThread);

	toThread = copyThread(s, fromThread, 0);
	leave(s);

	if (DEBUG_THREADS) {
		fprintf(stderr, "%d] GC_copyCurrentThread from="FMTPTR" to="FMTPTR"\n",
				PTHREAD_NUM, (uintptr_t) fromThread, (uintptr_t) toThread);

		fprintf(stderr, "   fromThread stack:\n");
		displayThread(s, (GC_thread)fromThread, stderr);
		fprintf(stderr, "   toThread stack:\n");
		displayThread(s, (GC_thread)toThread, stderr);
	}

	assert (s->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);

	s->savedThread[PTHREAD_NUM] = pointerToObjptr((pointer) toThread - offsetofThread(s), s->umheap.start);
	fprintf(stderr, "%d]   %s "RED("setting savedThread")" to "FMTPTR"\n",
			PTHREAD_NUM, __FUNCTION__, s->savedThread[PTHREAD_NUM]);
}

pointer GC_copyThread(GC_state s, pointer p) {
	GC_thread fromThread;
	GC_thread toThread;

	enter(s);
	fromThread = (GC_thread) (p + offsetofThread(s));
	if (DEBUG_THREADS) {
		fprintf(stderr, "%d] GC_copyThread from="FMTPTR" to="YELLOW("not-available-yet")"\n",
				PTHREAD_NUM, (uintptr_t) fromThread);
	}
	if ((GC_thread)(s->currentThread[PTHREAD_NUM]) == fromThread) {
		fprintf(stderr,
				RED("%d]   looks like you are trying to copy the currentThread with copyThread?\n"),
				PTHREAD_NUM);
		fromThread->currentFrame = s->currentFrame[PTHREAD_NUM];
	}
	toThread = copyThread(s, fromThread, 0);
	leave(s);
	if (DEBUG_THREADS) {
		fprintf(stderr, "%d] GC_copyThread from="FMTPTR" to="FMTPTR"\n",
				PTHREAD_NUM,
				(uintptr_t) fromThread, (uintptr_t) toThread);
		fprintf(stderr, "   fromThread stack:\n");
		displayThread(s, (GC_thread)fromThread, stderr);
		fprintf(stderr, "   toThread stack:\n");
		displayThread(s, (GC_thread)toThread, stderr);
	}

	return ((pointer) toThread - offsetofThread(s));
}

