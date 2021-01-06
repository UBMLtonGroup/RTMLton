#include "../gc.h"

/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void displayThread (GC_state s,
                    GC_thread thread,
                    FILE *stream) {
	fprintf(stream, "\t\texnStack = "FMTPTR"\n", (uintptr_t)thread->exnStack);
	fprintf(stream, "\t\tbytesNeeded = %"PRIuMAX"\n", (uintmax_t)thread->bytesNeeded);
	fprintf(stream,
			"\t\tcurrentFrame = "FMTPTR"\n\t\tfirstFrame = "FMTPTR"\n",
			(uintptr_t)thread->currentFrame,
			(uintptr_t)thread->firstFrame);


	GC_UM_Chunk c = (GC_UM_Chunk)(thread->firstFrame - GC_HEADER_SIZE);
	assert(c->prev_chunk == NULL);


	int count = 0, depth = 0, found = 0;
	while (c->next_chunk != NULL) {
		if (found == 0) {
			if (c != (GC_UM_Chunk)(thread->currentFrame - GC_HEADER_SIZE))
				depth++;
			else
				found = 1;
		}
		count++;
		c = c->next_chunk;
	}

	fprintf(stream, "\t\tstack size = %d chunks\n", count);
	fprintf(stream, "\t\tstack depth = %d chunks\n", depth);


#if 0
	displayStack (s, (GC_stack)(objptrToPointer (thread->stack, s->heap.start)),
				  stream);
#endif

}

size_t sizeofThread (GC_state s) {
  size_t res;

  res = GC_NORMAL_HEADER_SIZE + sizeof (struct GC_thread);
  res = align (res, s->alignment);
  if (DEBUG) {
    size_t check;
    uint16_t bytesNonObjptrs, numObjptrs;

    splitHeader (s, GC_THREAD_HEADER, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
    check = GC_NORMAL_HEADER_SIZE + (bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE));
    if (DEBUG_DETAILED)
      fprintf (stderr,
               "sizeofThread: res = %"PRIuMAX"  check = %"PRIuMAX"\n",
               (uintmax_t)res, (uintmax_t)check);
    assert (check == res);
  }
  assert (isAligned (res, s->alignment));
  return res;
}

size_t offsetofThread (GC_state s) {
  return (sizeofThread (s)) - (GC_NORMAL_HEADER_SIZE + sizeof (struct GC_thread));
}
