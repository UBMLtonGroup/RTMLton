#include "../gc.h"

/* Copyright (C) 2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

/* newObject (s, header, bytesRequested, allocInOldGen)
 *
 * Allocate a new object in the heap.
 * bytesRequested includes the size of the header.
 */
pointer newObject(GC_state s,
				  GC_header header,
				  size_t bytesRequested,
				  bool allocInOldGen) {
	return newUMObject(s, header, bytesRequested, allocInOldGen);
}

pointer newUMObject(GC_state s,
					GC_header header,
					size_t bytesRequested,
					__attribute__ ((unused)) bool allocInOldGen) {
	die("invalid codepath : newUMObject\n");
}

/* this variable is declared by the c-codegen. if you
 * -keep g, you will find it in the ".0.c" file
 */
extern uint32_t frameLayouts_len;

/* the UM stack is more like a stacklet. we allocate N chunks,
 * where N corresponds to the number of anticipated stack frames.
 * in the first rev, we initially allocate as many chunks as there
 * are frameLayouts. this is an arbitrary choice. each stacklet
 * is prefaced with a mlton stackheader

 TODO growStack_um to add chunks to an existing stack

 */

// XXX jcm remove eventually
#ifdef STACK_GC_SANITY
GC_UM_Chunk stack_list[100000];
unsigned int stack_list_end = 0;
#endif

objptr newStack_um(GC_state s, size_t stackSizeInBytes, size_t *stackSizeInChunks) {
	pointer um_stack;
	uint32_t need_chunks = 20; //frameLayouts_len;

	if (DEBUG_STACKS)
		fprintf(stderr, "%d] newStack_um(stackSizeInBytes=%d) -> chunksneeded=%d maxFrameSize=%d chunkSize=%d\n",
				PTHREAD_NUM,
				(int)stackSizeInBytes,
				(int)need_chunks,
				(int)s->maxFrameSize,
				(int)sizeof(struct GC_UM_Chunk));

	if (stackSizeInChunks)
		*stackSizeInChunks = need_chunks;

	assert(s->maxFrameSize <= UM_CHUNK_PAYLOAD_SIZE);

	/* Reserve the allocation before actually allocating.
	 * Will block if not enough chunks available.
	 */
	reserveAllocation(s, need_chunks);
	um_stack = UM_Object_alloc(s, need_chunks, GC_STACK_HEADER, GC_NORMAL_HEADER_SIZE);

	if (DEBUG_STACKS) {
#ifdef STACK_GC_SANITY
		/* UM_Object_alloc returns a pointer that is +GC_NORMAL_HEADER_SIZE
		 * bytes ahead of the actual GC_UM_Chunk object.
		 */
		GC_UM_Chunk x = (GC_UM_Chunk) (um_stack - GC_NORMAL_HEADER_SIZE);
		for ( ; x ; x = x->next_chunk) {
			stack_list[stack_list_end] = x;
			stack_list_end++;
		}
#endif
		fprintf(stderr, FMTPTR " = newStack_um (chunks=%"PRIuMAX")\n",
				(uintptr_t) um_stack,
				(uintmax_t) need_chunks);
	}

	return pointerToObjptr(um_stack, s->umheap.start);
}

GC_stack newStack(GC_state s,
				  size_t reserved,
				  bool allocInOldGen) {
	fprintf(stderr, RED("*** warn newStack should not be called\n"));
	die("newstack called");

	return NULL;
}

GC_thread newThread(GC_state s, size_t stackSize) {
	GC_thread thread;
	pointer res;

	C_Size_t numchunks = (sizeofThread(s) < UM_CHUNK_PAYLOAD_SIZE) ? 1 : 2;
	assert(sizeofThread(s) < UM_CHUNK_PAYLOAD_SIZE); // TODO we should size chunk so it fits

	/*Reserve the allocation before actually allocating.
	 * Will block if not enough chunks available.
	 */
	reserveAllocation(s, numchunks);

	res = UM_Object_alloc(s, numchunks, GC_THREAD_HEADER, GC_NORMAL_HEADER_SIZE);

	// offsetofThread should be 0
	thread = (GC_thread) (res + offsetofThread(s));
	thread->bytesNeeded = 0;
	thread->exnStack = BOGUS_EXN_STACK;
	thread->firstFrame = newStack_um(s, stackSize, &(thread->stackSizeInChunks));
	thread->currentFrame = BOGUS_OBJPTR; //init-world first thread will do: thread->firstFrame;
	thread->stackDepth = 0;
	thread->markCycles = 0;

	if (DEBUG_THREADS)
		fprintf(stderr, "%d]      newThread(stackSize=%"PRIuMAX") = "FMTPTR"\n",
				PTHREAD_NUM, (uintmax_t)stackSize, (uintptr_t) thread);


	return thread;
}

static inline void setFrontier(GC_state s, pointer p,
			       ARG_USED_FOR_ASSERT size_t bytes) {
	p = alignFrontier(s, p);
	assert ((size_t)(p - s->frontier) <= bytes);
	GC_profileAllocInc(s, (size_t)(p - s->frontier));
	s->cumulativeStatistics.bytesAllocated += (size_t)(p - s->frontier);
	s->frontier = p;
	assert (s->frontier <= s->limitPlusSlop);
}
