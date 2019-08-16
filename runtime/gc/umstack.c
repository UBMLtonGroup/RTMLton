/*
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void um_displayStack (__attribute__ ((unused)) GC_state s,
                   GC_stack stack,
                   FILE *stream) {
	return;
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

    GC_UM_Chunk f = (GC_UM_Chunk)from->firstFrame;
    GC_UM_Chunk t = (GC_UM_Chunk)to->firstFrame;
    to->currentFrame = BOGUS_OBJPTR;

    if (DEBUG_CCODEGEN)
        fprintf(stderr, "%08x -> %08x init\n", (unsigned int) f, (unsigned int)t);

    for( ; f ; f = (GC_UM_Chunk)f->next_chunk, t = (GC_UM_Chunk)t->next_chunk) {
        GC_memcpy((pointer)f, (pointer)t, copyamt);
        if (from->currentFrame == (objptr)f) {
            if (DEBUG_CCODEGEN) {
		fprintf(stderr, "found cf: ");
		fprintf(stderr, "%08x -> %08x\n", (unsigned int) f, (unsigned int)t);
            }
            to->currentFrame = (objptr)t;
        }
        if (to->currentFrame == BOGUS_OBJPTR) {
            if (DEBUG_CCODEGEN)
                fprintf(stderr, "%08x -> %08x\n", (unsigned int) f, (unsigned int)t);
        }
        cc++;
    }

    if (DEBUG_CCODEGEN)
	fprintf(stderr, YELLOW("copyStacklet") " [Thr %d] copied %d chunks,"
					      " old-first %08x"
					      " old-cur %08x"
					      " new-first %08x"
					      " new-cur %08x\n",
		PTHREAD_NUM, cc,
		(unsigned int)from->firstFrame,
		(unsigned int)from->currentFrame,
		(unsigned int)to->firstFrame,
		(unsigned int)to->currentFrame);


    if (to->currentFrame == BOGUS_OBJPTR)
        die(RED("failed to figure out what currentFrame is on copied stack"));

    assert (f == NULL && t == NULL);
}

size_t um_offsetofStack (GC_state s) {
    return GC_STACK_HEADER_SIZE;
}
