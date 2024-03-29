/*
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */
#include "../gc.h"

void um_displayStack (__attribute__ ((unused)) GC_state s,
                   GC_stack stack,
                   FILE *stream) {
	um_dumpStack(s);
	return;
}


void um_dumpFrame (GC_state s, objptr frame, GC_returnAddress raoverride) {
	unsigned int i;
	GC_returnAddress returnAddress;
	GC_frameLayout frameLayout;
	GC_frameOffsets frameOffsets;
	GC_UM_Chunk chunk = (GC_UM_Chunk)frame;

	assert (chunk->sentinel == UM_STACK_SENTINEL);

	returnAddress = *(GC_returnAddress*)(chunk->ml_object + chunk->ra);
	if (raoverride > 0) {
		returnAddress = raoverride;
	}

	fprintf (stderr, "%d] frame:  chunkAddr = "FMTPTR"  return address = "FMTRA" (%d) (ra=%d)\n",
			PTHREAD_NUM,
			(uintptr_t)chunk, (uintptr_t) returnAddress, (int)returnAddress,
			(int)chunk->ra);

	if (chunk->ra == 0) {
		fprintf(stderr, "%d]   ra=0, so cant decode this frame\n", PTHREAD_NUM);
		return;
	}

	frameLayout = getFrameLayoutFromReturnAddress (s, returnAddress);
	frameOffsets = frameLayout->offsets;

	fprintf(stderr, "%d]   frame: kind %s size %"PRIx16"\n",
			PTHREAD_NUM, (frameLayout->kind==C_FRAME)?"C_FRAME":"ML_FRAME", frameLayout->size);

	for (i = 0 ; i < frameOffsets[0] ; ++i) {
		uintptr_t x = (uintptr_t)(&(chunk->ml_object)) + frameOffsets[i + 1] + s->alignment;
		unsigned int xv = *(objptr*)x;

		fprintf(stderr, "%d]    offset 0x%"PRIx16" (%d) stackaddress "FMTOBJPTR" objptr "FMTOBJPTR"\n",
				PTHREAD_NUM,
				frameOffsets[i + 1], frameOffsets[i + 1],
				(int)x,
				xv);
	}

}


void um_dumpStack (GC_state s) {
	if (PTHREAD_NUM == 1) return;

	GC_thread thread = (GC_thread)s->currentThread[PTHREAD_NUM];

	GC_UM_Chunk top = (GC_UM_Chunk)(s->currentFrame[PTHREAD_NUM] - GC_HEADER_SIZE);
	GC_UM_Chunk bottom = (GC_UM_Chunk)(thread->firstFrame - GC_HEADER_SIZE);
	GC_UM_Chunk chunk = top;

	do {
		um_dumpFrame(s, (objptr)chunk, 0);
		if (bottom == chunk) return;
		chunk = chunk->prev_chunk;
	} while (chunk);
}

bool um_isStackEmpty (GC_stack stack) {
  return 0 == stack->used;
}


/* Pointer to the bottommost chunk on the stacklet / umstack. the frame
 * start is the start of the chunk + header.
 */
pointer um_getStackBottom (ARG_USED_FOR_ASSERT GC_state s, objptr stack) {
    pointer res;

    res = ((pointer)stack) + GC_STACK_HEADER_SIZE;

    if (DEBUG_CCODEGEN)
		fprintf(stderr, "getUMStackBottom is aligned? %zu %zu %zu\n",
			(size_t)res, s->alignment,
			(size_t)res / s->alignment);
    assert (isAligned ((size_t)res, s->alignment));
    return res;
}

/* Pointer to the topmost word in use on the stack. */
pointer um_getStackTop (GC_state s, objptr stack) {
    pointer res = NULL;
    die("not implemented");
    //res = getStackBottom (s, stack) + stack->used;
    //assert (isAligned ((size_t)res, s->alignment));
    return res;
}


/* given two thread structs, copy the stacklet (list of chunks)
 * from the first to the second. figure out what the correct
 * currentFrame is for the destination thread and set it.
 *
 * if you are copying the stack of the current thread then you
 * should copy gcstate->currentframe[x] into fromthread->currentframe
 * to make sure it is sync'd correctly.
 *
 * both stacklets must be already allocated
 * both stacklets must be the same number of chunks (asserted)
 */
void um_copyStack (GC_state s, GC_thread from, GC_thread to) {
    // we copy everything except the prev/next pointers
    // note this strategy /requires/ that the prev/next ptrs
    // are the last elements in the GC_UM_Chunk struct
    size_t copyamt = sizeof(struct GC_UM_Chunk) -
                     2 * sizeof(struct GC_UM_Chunk*);

    int cc = 0;

    GC_UM_Chunk f = (GC_UM_Chunk)(from->firstFrame - GC_HEADER_SIZE);
    GC_UM_Chunk t = (GC_UM_Chunk)(to->firstFrame - GC_HEADER_SIZE);
    to->currentFrame = BOGUS_OBJPTR;
	to->exnStack = BOGUS_EXN_STACK;

    if (DEBUG_CCODEGEN)
        fprintf(stderr, "%d] um_copyStack from:%08x -> to:%08x\n", PTHREAD_NUM, (unsigned int) f, (unsigned int)t);

    for( ; f ; f = (GC_UM_Chunk)f->next_chunk, t = (GC_UM_Chunk)t->next_chunk) {
    	assert (f->sentinel == UM_STACK_SENTINEL);
    	assert (t->sentinel == UM_STACK_SENTINEL);

        GC_memcpy((pointer)f, (pointer)t, copyamt);
        if (DEBUG_CCODEGEN)
	        fprintf(stderr, "%d]    raoffset %d ra %d\n", PTHREAD_NUM, (int)f->ra, f->ml_object[f->ra + GC_HEADER_SIZE]);

        if (from->exnStack == (objptr)f + GC_HEADER_SIZE) {
			if (DEBUG_CCODEGEN) {
				fprintf(stderr, GREEN("found exnStack: from:%08x -> to:%08x (handler %d, ra %d)\n"),
						(unsigned int) f,
						(unsigned int) t,
						(int)f->handler,
						(int)*(uintptr_t*)(f->ml_object + f->ra + GC_HEADER_SIZE));
			}
			to->exnStack = (objptr)t + GC_HEADER_SIZE;
        }
        if (from->currentFrame - GC_HEADER_SIZE == (objptr)f) {
            if (DEBUG_CCODEGEN) {
				fprintf(stderr, "found cf at from:%08x -> to:%08x\n", (unsigned int) f, (unsigned int)t);
            }
            to->currentFrame = (objptr)t + GC_HEADER_SIZE;
            f = f->next_chunk;
            t = t->next_chunk;
			GC_memcpy((pointer)f, (pointer)t, copyamt);
			break;
        }
        if (to->currentFrame == BOGUS_OBJPTR) {
            if (DEBUG_CCODEGEN)
                fprintf(stderr, "did not find cf yet! from:%08x -> to:%08x\n", (unsigned int) f, (unsigned int)t);
        }
        cc++;
    }

	to->stackDepth = from->stackDepth;

	if (DEBUG_CCODEGEN) {
    	if (to->exnStack == BOGUS_EXN_STACK) {
    		fprintf(stderr, YELLOW("No exnStack found in from thread\n"));
    	}
		fprintf(stderr, YELLOW("copyStacklet") " [Thr %d] copied %d chunks,"
						" old-first %08x"
						" old-cur %08x"
						" new-first %08x"
						" new-cur %08x\n",
				PTHREAD_NUM, cc,
				(unsigned int) from->firstFrame,
				(unsigned int) from->currentFrame,
				(unsigned int) to->firstFrame,
				(unsigned int) to->currentFrame);
	}

    if (to->currentFrame == BOGUS_OBJPTR)
        die(RED("failed to figure out what currentFrame is on copied stack"));

    //assert (f == NULL && t == NULL);
}

size_t um_offsetofStack (GC_state s) {
    return GC_STACK_HEADER_SIZE;
}
