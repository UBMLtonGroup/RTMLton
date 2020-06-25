#include "../gc.h"

/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void switchToThread (GC_state s, objptr op) {
	if (DEBUG_THREADS) {
		//GC_thread thread;
		//GC_stack stack;

		//thread = (GC_thread)(objptrToPointer (op, s->umheap.start)
		//					 + offsetofThread (s));
		//stack = (GC_stack)(objptrToPointer (thread->stack, s->umheap.start));

		fprintf (stderr, "switchToThread ("FMTOBJPTR")\n",
				op);
	}

	s->currentThread[PTHREAD_NUM] = op;
	setGCStateCurrentThreadAndStack (s);  // sets s->currentFrame, s->exnStack
}

void GC_switchToThread (GC_state s, pointer p, size_t ensureBytesFree) {
	if (DEBUG_THREADS)
		fprintf (stderr, GREEN("GC_switchToThread")" ("FMTPTR", %"PRIuMAX")\n",
			(uintptr_t)p, (uintmax_t)ensureBytesFree);
	if (FALSE) {
		/* This branch is slower than the else branch, especially
		 * when debugging is turned on, because it does an invariant
		 * check on every thread switch.
		 * So, we'll stick with the else branch for now.
		 */
		die("dont do this branch, wheres the stack saving?");
		enter (s);
		getThreadCurrent(s)->bytesNeeded = ensureBytesFree;
		switchToThread (s, pointerToObjptr(p, s->umheap.start));
		decAtomic(s); /* s->atomicState--; */
		switchToSignalHandlerThreadIfNonAtomicAndSignalPending (s);
		ensureInvariantForMutator (s, FALSE);
		assert (invariantForMutatorFrontier(s));
		assert (invariantForMutatorStack(s));
		leave (s);
	} else {
		/* BEGIN: enter(s); */
		//getStackCurrent(s)->used = sizeofGCStateCurrentStackUsed (s);

		if (DEBUG_THREADS)
			fprintf(stderr, YELLOW("CurFrame CHECK")
			    " t->cf %08x s->cf[%d] %08x\n",
				(unsigned int)getThreadCurrent(s)->currentFrame , PTHREAD_NUM,
				(unsigned int)s->currentFrame[PTHREAD_NUM]);

		beginAtomic (s);
		/* END: enter(s); */

		// stacklet, save currentFrame on thread swap
		getThreadCurrent(s)->currentFrame = s->currentFrame[PTHREAD_NUM];
		getThreadCurrent(s)->exnStack = s->exnStack[PTHREAD_NUM];
		getThreadCurrent(s)->bytesNeeded = ensureBytesFree;
		getThreadCurrent(s)->stackDepth = s->stackDepth[PTHREAD_NUM];

		switchToThread (s, pointerToObjptr(p, s->umheap.start));
		decAtomic(s); /* s->atomicState--; */
		switchToSignalHandlerThreadIfNonAtomicAndSignalPending (s);
		/* BEGIN: ensureInvariantForMutator */
		if (not (invariantForMutatorFrontier(s))
			or not (invariantForMutatorStack(s))) {
			maybe_growstack(s, (GC_thread)p, FALSE);
		}
		/* END: ensureInvariantForMutator */
		/* BEGIN: leave(s); */
		endAtomic (s);
		/* END: leave(s); */
	}
	assert (invariantForMutatorFrontier(s));
	assert (invariantForMutatorStack(s));
}
