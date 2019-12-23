/* Copyright (C) 2011-2012 Matthew Fluet.
 * Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

GC_thread copyThread (GC_state s, GC_thread from, size_t used) {
	GC_thread to;

	if (DEBUG_THREADS)
		fprintf (stderr, "copyThread ("FMTPTR") my_pthread=%u\n", (uintptr_t)from, PTHREAD_NUM);

	/* newThread may do a GC, which invalidates from.
	 * Hence we need to stash from someplace that the GC can find it.
	 */
	assert (s->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);
	s->savedThread[PTHREAD_NUM] = pointerToObjptr((pointer)from - offsetofThread (s), s->umheap.start);
	to = newThread (s, alignStackReserved(s, used));
	from = (GC_thread)(objptrToPointer(s->savedThread[PTHREAD_NUM], s->umheap.start) + offsetofThread (s));
	s->savedThread[PTHREAD_NUM] = BOGUS_OBJPTR;
	if (DEBUG_THREADS) {
		fprintf (stderr, FMTPTR" = copyThread ("FMTPTR")\n",
				(uintptr_t)to, (uintptr_t)from);
	}

	to->bytesNeeded = from->bytesNeeded; // TODO what does this do in stacklets?
	//to->exnStack = from->exnStack; // this will be adjusted in um_copyStack below

	um_copyStack(s, from, to); // note we pass in GC_threads not the actual stacklets

	return to;
}

void GC_copyCurrentThread (GC_state s, bool b) {
	GC_thread fromThread;
	//GC_stack fromStack;
	GC_thread toThread;
	//LOCAL_USED_FOR_ASSERT GC_stack toStack;

	if (DEBUG_THREADS)
		fprintf (stderr, GREEN("GC_copyCurrentThread\n"));
	enter (s);
	fromThread = (GC_thread)(objptrToPointer(s->currentThread[PTHREAD_NUM], s->umheap.start)
							 + offsetofThread (s));
	//fromStack = (GC_stack)(objptrToPointer(fromThread->stack, s->umheap.start));
	toThread = copyThread (s, fromThread, 0);
	//toStack = (GC_stack)(objptrToPointer(toThread->stack, s->umheap.start));
	//assert (toStack->reserved == alignStackReserved (s, toStack->used));
	leave (s);
	if (DEBUG_THREADS)
		fprintf (stderr, FMTPTR" = GC_copyCurrentThread\n", (uintptr_t)toThread);
	assert (s->savedThread[PTHREAD_NUM] == BOGUS_OBJPTR);

	if (b)
		s->savedThread[PTHREAD_NUM] = pointerToObjptr((pointer)toThread - offsetofThread (s), s->umheap.start);
}

pointer GC_copyThread (GC_state s, pointer p) {
	GC_thread fromThread;
	//GC_stack fromStack;
	GC_thread toThread;
	//LOCAL_USED_FOR_ASSERT GC_stack toStack;

	if (DEBUG_THREADS)
		fprintf (stderr, GREEN("GC_copyThread")" ("FMTPTR", pthread=%u)\n", (uintptr_t)p, PTHREAD_NUM);
	enter (s);
	fromThread = (GC_thread)(p + offsetofThread (s));
	//fromStack = (GC_stack)(objptrToPointer(fromThread->stack, s->umheap.start));
	toThread = copyThread (s, fromThread, 0);
	//toStack = (GC_stack)(objptrToPointer(toThread->stack, s->umheap.start));
	//assert (toStack->reserved == alignStackReserved (s, toStack->used));
	leave (s);
	if (DEBUG_THREADS)
		fprintf (stderr, FMTPTR" = GC_copyThread ("FMTPTR")\n",
			(uintptr_t)toThread, (uintptr_t)fromThread);
	return ((pointer)toThread - offsetofThread (s));
}

