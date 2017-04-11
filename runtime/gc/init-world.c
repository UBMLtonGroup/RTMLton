/* Copyright (C) 2011-2012,2014 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

/* ---------------------------------------------------------------- */
/*                          Initialization                          */
/* ---------------------------------------------------------------- */

size_t sizeofInitialBytesLive (GC_state s) {
  uint32_t i;
  size_t dataBytes;
  size_t total;

  total = 0;
  for (i = 0; i < s->vectorInitsLength; ++i) {
    dataBytes =
      s->vectorInits[i].elementSize
      * s->vectorInits[i].length;
    total += align (GC_ARRAY_HEADER_SIZE
                    + ((dataBytes < OBJPTR_SIZE)
                       ? OBJPTR_SIZE
                       : dataBytes),
                    s->alignment);
  }
  return total;
}

void initVectors (GC_state s) {
  struct GC_vectorInit *inits;
  pointer frontier;
  uint32_t i;

  assert (isFrontierAligned (s, s->frontier));
  inits = s->vectorInits;
  frontier = s->frontier;
  for (i = 0; i < s->vectorInitsLength; i++) {
    size_t elementSize;
    size_t dataBytes;
    size_t objectSize;
    uint32_t typeIndex;

    elementSize = inits[i].elementSize;
    dataBytes = elementSize * inits[i].length;
    objectSize = align (GC_ARRAY_HEADER_SIZE
                        + ((dataBytes < OBJPTR_SIZE)
                           ? OBJPTR_SIZE
                           : dataBytes),
                        s->alignment);
    assert (objectSize <= (size_t)(s->heap.start + s->heap.size - frontier));
    *((GC_arrayCounter*)(frontier)) = 0;
    frontier = frontier + GC_ARRAY_COUNTER_SIZE;
    *((GC_arrayLength*)(frontier)) = inits[i].length;
    frontier = frontier + GC_ARRAY_LENGTH_SIZE;
    switch (elementSize) {
    case 1:
      typeIndex = WORD8_VECTOR_TYPE_INDEX;
      break;
    case 2:
      typeIndex = WORD16_VECTOR_TYPE_INDEX;
      break;
    case 4:
      typeIndex = WORD32_VECTOR_TYPE_INDEX;
      break;
    case 8:
      typeIndex = WORD64_VECTOR_TYPE_INDEX;
      break;
    default:
      die ("unknown element size in vectorInit: %"PRIuMAX"",
           (uintmax_t)elementSize);
    }
    *((GC_header*)(frontier)) = buildHeaderFromTypeIndex (typeIndex);
    frontier = frontier + GC_HEADER_SIZE;
    s->globals[inits[i].globalIndex] = pointerToObjptr(frontier, s->heap.start);
    if (DEBUG_DETAILED)
      fprintf (stderr, "allocated vector at "FMTPTR"\n",
               (uintptr_t)(s->globals[inits[i].globalIndex]));
    memcpy (frontier, inits[i].words, dataBytes);
    frontier += objectSize - GC_ARRAY_HEADER_SIZE;
  }
  if (DEBUG_DETAILED)
    fprintf (stderr, "frontier after string allocation is "FMTPTR"\n",
             (uintptr_t)frontier);
  GC_profileAllocInc (s, (size_t)(frontier - s->frontier));
  s->cumulativeStatistics.bytesAllocated += (size_t)(frontier - s->frontier);
  assert (isFrontierAligned (s, frontier));
  s->frontier = frontier;
}

void initWorld (GC_state s) {
  uint32_t i;
  pointer start;
  GC_thread thread;

  for (i = 0; i < s->globalsLength; ++i)
    s->globals[i] = BOGUS_OBJPTR;
  s->lastMajorStatistics.bytesLive = sizeofInitialBytesLive (s);

  /* alloc um first so normal heap can expand without overrunning us */


#define MEGABYTES 1024*1024
#define MEM_AVAILABLE 1024
  size_t avail_mem = s->controls.maxHeap ? s->controls.maxHeap : (MEM_AVAILABLE * MEGABYTES);
  createUMHeap (s, &s->umheap, avail_mem, avail_mem);

  createUMHeap (s, &s->umarheap, avail_mem, avail_mem);

  createHeap (s, &s->heap, 100*MEGABYTES, 100*MEGABYTES);

  createHeap (s, &s->infHeap, 100*MEGABYTES, 100*MEGABYTES);
//              sizeofHeapDesired (s, s->lastMajorStatistics.bytesLive, 0),
//               s->lastMajorStatistics.bytesLive);
//              sizeofHeapDesired (s, s->lastMajorStatistics.bytesLive, 0),
//              s->lastMajorStatistics.bytesLive);
  s->gc_module = GC_UM;
  setCardMapAndCrossMap (s);
  start = alignFrontier (s, s->heap.start);
  s->umarfrontier = s->umarheap.start;
  s->frontier = start;
  s->infFrontier = s->infHeap.start;
  s->limitPlusSlop = s->heap.start + s->heap.size;
  s->limit = s->limitPlusSlop - GC_HEAP_LIMIT_SLOP;
  initVectors (s);
  assert ((size_t)(s->frontier - start) <= s->lastMajorStatistics.bytesLive);

s->heap.oldGenSize = (size_t)(s->frontier - s->heap.start);
  setGCStateCurrentHeap (s, 0, 0);

  GC_UM_Chunk next_chunk = NULL;
  next_chunk = allocNextChunk(s, &(s->umheap));
  next_chunk->next_chunk = NULL;
  s->umfrontier = (Pointer) next_chunk->ml_object;


  thread = newThread (s, sizeofStackInitialReserved (s)); // defaults to pri 0

  switchToThread (s, pointerToObjptr((pointer)thread - offsetofThread (s), s->heap.start));
  if (DEBUG_MEM) {
      fprintf(stderr, "UMFrontier start: "FMTPTR"\n", (uintptr_t)(s->umfrontier));
  }
}
