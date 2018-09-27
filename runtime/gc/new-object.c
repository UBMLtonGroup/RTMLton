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
pointer newObject (GC_state s,
                   GC_header header,
                   size_t bytesRequested,
                   bool allocInOldGen) {

    return newUMObject(s, header, bytesRequested, allocInOldGen);
  pointer frontier;
  pointer result;

  assert (isAligned (bytesRequested, s->alignment));
  assert (allocInOldGen
          ? hasHeapBytesFree (s, bytesRequested, 0)
          : hasHeapBytesFree (s, 0, bytesRequested));
  if (allocInOldGen) {
    frontier = s->heap.start + s->heap.oldGenSize;
    s->heap.oldGenSize += bytesRequested;
    s->cumulativeStatistics.bytesAllocated += bytesRequested;
  } else {
    if (DEBUG_DETAILED)
      fprintf (stderr, "frontier changed from "FMTPTR" to "FMTPTR"\n",
               (uintptr_t)s->frontier,
               (uintptr_t)(s->frontier + bytesRequested));
    frontier = s->frontier;
    s->frontier += bytesRequested;
  }
  GC_profileAllocInc (s, bytesRequested);
  *((GC_header*)frontier) = header;
  result = frontier + GC_NORMAL_HEADER_SIZE;
  assert (isAligned ((size_t)result, s->alignment));
  if (DEBUG)
    fprintf (stderr, FMTPTR " = newObject ("FMTHDR", %"PRIuMAX", %s)\n",
             (uintptr_t)result,
             header,
             (uintmax_t)bytesRequested,
             boolToString (allocInOldGen));
  return result;
}

pointer newUMObject (GC_state s,
                     GC_header header,
                     size_t bytesRequested,
                     __attribute__ ((unused)) bool allocInOldGen) {
    pointer frontier;
    pointer result;

    frontier = s->frontier;
    s->frontier += bytesRequested;
    *((GC_header*)frontier) = header;
    result = frontier + GC_NORMAL_HEADER_SIZE;
    return result;
}

GC_stack newStack (GC_state s,
                   size_t reserved,
                   bool allocInOldGen) {
  GC_stack stack;
  reserved = 50 * 1024 * 1024;
  assert (isStackReservedAligned (s, reserved));
  if (reserved > s->cumulativeStatistics.maxStackSize)
    s->cumulativeStatistics.maxStackSize = reserved;
  stack = (GC_stack)(newUMObject (s, GC_STACK_HEADER,
                                  sizeofStackWithHeader (s, reserved),
                                  allocInOldGen));
  stack->reserved = reserved;
  stack->used = 0;
  if (DEBUG_STACKS)
    fprintf (stderr, FMTPTR " = newStack (%"PRIuMAX")\n",
             (uintptr_t)stack,
             (uintmax_t)reserved);
  return stack;
}

GC_thread newThread (GC_state s, size_t reserved) {
  GC_stack stack;
  GC_thread thread;
  pointer res;

  if (DEBUG)
	  fprintf(stderr, "newThread\n");

  assert (isStackReservedAligned (s, reserved));
  ensureHasHeapBytesFree (s, 0, sizeofStackWithHeader (s, reserved) + sizeofThread (s));
  stack = newStack (s, reserved, FALSE);

  C_Size_t numChunks = (sizeofThread(s) < UM_CHUNK_PAYLOAD_SIZE) ? 1 : 2;
  res = UM_Object_alloc(s, numChunks,GC_THREAD_HEADER,GC_NORMAL_HEADER_SIZE);


  /*res = newUMObject (s, GC_THREAD_HEADER,
                     sizeofThread (s),
                     FALSE);*/
  thread = (GC_thread)(res + offsetofThread (s));
  thread->bytesNeeded = 0;
  thread->exnStack = BOGUS_EXN_STACK;
  thread->stack = pointerToObjptr((pointer)stack, s->heap.start);
  if (DEBUG_THREADS)
    fprintf (stderr, FMTPTR" = newThreadOfSize (%"PRIuMAX")\n",
             (uintptr_t)thread, (uintmax_t)reserved);

  return thread;
}

static inline void setFrontier (GC_state s, pointer p,
                                ARG_USED_FOR_ASSERT size_t bytes) {
  p = alignFrontier (s, p);
  assert ((size_t)(p - s->frontier) <= bytes);
  GC_profileAllocInc (s, (size_t)(p - s->frontier));
  s->cumulativeStatistics.bytesAllocated += (size_t)(p - s->frontier);
  s->frontier = p;
  assert (s->frontier <= s->limitPlusSlop);
}
