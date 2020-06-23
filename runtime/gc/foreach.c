#include "../gc.h"
/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wcast-qual" /*squishing wcast qual for callIfIsObjptr (s, f, (objptr *)&s->callFromCHandlerThread);*/

void callIfIsObjptr (GC_state s, GC_foreachObjptrFun f, objptr *opp) {
    if (isObjptr (*opp)) {
    	if (is_on_um_heap(s, (Pointer)*opp))
	        f (s, opp);
        return;
    }

    if (DEBUG_MEM)
        fprintf(stderr, "  callIfIsObjptr: Not objptr 0x%x\n", *opp);
}


void foreachGlobalThreadObjptr(GC_state s, GC_foreachObjptrFun f) {

	//if (DEBUG_DETAILED)
	//  fprintf (stderr, "%d] foreachGlobal threads\n", PTHREAD_NUM);


	// in rtmlton i think it is safe to disable this feature
	// where you can call SML /from/ C. our research is focused (?) on
	// pure SML systems.

	if (DEBUG_DETAILED)
		fprintf(stderr, "%d] callFromCHandlerThread: "FMTPTR"\n", PTHREAD_NUM, s->callFromCHandlerThread);

	callIfIsObjptr(s, f, &s->callFromCHandlerThread);

	if (DEBUG_DETAILED)
		fprintf(stderr, "%d] currentThread: "FMTPTR"\n", PTHREAD_NUM, s->currentThread[PTHREAD_NUM]);

	callIfIsObjptr(s, f, &s->currentThread[PTHREAD_NUM]);

	if (DEBUG_DETAILED)
		fprintf(stderr, "%d] savedThread: "FMTPTR"\n", PTHREAD_NUM, s->savedThread[PTHREAD_NUM]);

	callIfIsObjptr(s, f, &s->savedThread[PTHREAD_NUM]);

	if (DEBUG_DETAILED)
		fprintf(stderr, "%d] signalHandlerThread: "FMTPTR"\n", PTHREAD_NUM, s->signalHandlerThread[PTHREAD_NUM]);

	callIfIsObjptr(s, f, &s->signalHandlerThread[PTHREAD_NUM]);
}


/* foreachGlobalObjptr (s, f)
 *
 * Apply f to each global object pointer into the heap.
 */
void foreachGlobalObjptr (GC_state s, GC_foreachObjptrFun f) {
  for (unsigned int i = 0; i < s->globalsLength; ++i) {
    if (DEBUG_DETAILED)
      fprintf (stderr, "%d] foreachGlobal %u\n", PTHREAD_NUM, i);
    callIfIsObjptr (s, f, &s->globals [i]);
  }
  
  //foreachGlobalThreadObjptr(s,f);
  
}


/* foreachObjptrInObject (s, p, f, skipWeaks)
 *
 * Applies f to each object pointer in the object pointed to by p.
 * Returns pointer to the end of object, i.e. just past object.
 *
 * If skipWeaks, then the object pointer in weak objects is skipped.
 */
pointer foreachObjptrInObject (GC_state s, pointer p,
                               GC_foreachObjptrFun f, bool skipWeaks) {
  if (DEBUG_MEM) {
      fprintf(stderr, "%d] foreach object in 0x%x\n", PTHREAD_NUM, (uintptr_t)p);
  }
  GC_header header;
  uint16_t bytesNonObjptrs = 0;
  uint16_t numObjptrs = 0;
  GC_objectTypeTag tag = ERROR_TAG;

  header = getHeader (p);
  splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);
  if (DEBUG_DETAILED)
    fprintf (stderr,
             "%d] foreachObjptrInObject ("FMTPTR")"
             "  header = "FMTHDR
             "  tag = %s"
             "  bytesNonObjptrs = %d"
             "  numObjptrs = %d\n", PTHREAD_NUM,
             (uintptr_t)p, header, objectTypeTagToString (tag),
             bytesNonObjptrs, numObjptrs);
  if (NORMAL_TAG == tag) {
	if (DEBUG_MEM) fprintf(stderr, "%d] "GREEN("marking normal\n"), PTHREAD_NUM);

      if(!isObjectOnUMHeap(s,p)) 
      {
        die("Non stack Object in old heap");
      }

      if (DEBUG_MEM)
          fprintf(stderr, "   foreachObjptrInObject, normal, bytesNonObjptrs: %d, "
                  "num ptrs: %d\n", bytesNonObjptrs, numObjptrs);


      for (int i=0; i<numObjptrs; i++) {
          pointer todo = UM_CPointer_offset(s, p, bytesNonObjptrs + i * OBJPTR_SIZE,
                                            OBJPTR_SIZE);

		if (*(objptr*)todo == (objptr)NULL) {
			fprintf(stderr, "%d] *TODO is null "FMTPTR"\n",
					PTHREAD_NUM, (uintptr_t)todo);
		} else {
			callIfIsObjptr(s, f, (objptr *) todo);
		}
      }

  } else if (WEAK_TAG == tag) {
	if (DEBUG_MEM) fprintf(stderr, "%d] "GREEN("marking weak\n"), PTHREAD_NUM);

  	p += bytesNonObjptrs;
    if (1 == numObjptrs) {
      if (not skipWeaks)
        callIfIsObjptr (s, f, (objptr*)p);
      p += OBJPTR_SIZE;
    }
  } else if (ARRAY_TAG == tag &&
             ((p >= s->umheap.start + s->umheap.size) ||
              (p < s->umheap.start))) {
    size_t bytesPerElement;
    size_t dataBytes;
    pointer last;
    GC_arrayLength numElements;

    if (DEBUG_MEM) fprintf(stderr, "%d] "GREEN("marking array (old heap)\n"), PTHREAD_NUM);

    numElements = getArrayLength (p);
    bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);
    dataBytes = numElements * bytesPerElement;
    if (dataBytes < OBJPTR_SIZE) {
      /* Very small (including empty) arrays have OBJPTR_SIZE bytes
       * space for the forwarding pointer.
       */
      dataBytes = OBJPTR_SIZE;
    } else if (0 == numObjptrs) {
      /* No objptrs to process. */
      ;
    } else {
      last = p + dataBytes;
      if (0 == bytesNonObjptrs)
        /* Array with only pointers. */
        for ( ; p < last; p += OBJPTR_SIZE)
          callIfIsObjptr (s, f, (objptr*)p);
      else {
        /* Array with a mix of pointers and non-pointers. */
        size_t bytesObjptrs;

        bytesObjptrs = numObjptrs * OBJPTR_SIZE;

        /* For each array element. */
        for ( ; p < last; ) {
          pointer next;

          /* Skip the non-pointers. */
          p += bytesNonObjptrs;
          next = p + bytesObjptrs;
          /* For each internal pointer. */
          for ( ; p < next; p += OBJPTR_SIZE)
            callIfIsObjptr (s, f, (objptr*)p);
        }
      }
      assert (p == last);
      p -= dataBytes;
    }
    p += alignWithExtra (s, dataBytes, GC_ARRAY_HEADER_SIZE);
  } else if (ARRAY_TAG == tag) {
	  if (DEBUG_MEM) fprintf(stderr, "%d] "GREEN("marking array (new heap)\n"), PTHREAD_NUM);

	  /* In an array object, the bytesNonObjptrs
       * field indicates the number of bytes of non heap-pointer data in a
       * single array element, while the numObjptrs field indicates the
 	   * number of heap pointers in a single array element. [object.h]
	   */

	  size_t bytesPerElement;
      size_t dataBytes, curBytePosition;
      pointer last;
	  GC_arrayLength numElements;
      numElements = getArrayLength (p);
      bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);
      dataBytes = numElements * bytesPerElement;

      if (0 == numObjptrs) {
      	  /* no objptrs to process */;
      } else {
		  last += 0;
		  curBytePosition = 0;
		  /* 1. find first leaf
		   * 2. if all ptrs:
		   *    2a. process ptrs until end of leaf
		   *    2b. go to leaf->next, repeat until done
		   * 3. if mixed, note structure of array (see array.h)
		   *    foreach element in array:
		   *        objptrs = element + bytesNonObjptrs
		   *        foreach objptr in objptrs:
		   *            apply f to objptr
		   *
		   * note: calling UM_Array_offset foreach element is
		   * inefficient.
		   */

		  if (0 == bytesNonObjptrs) {
			  pointer leaf = UM_walk_array_leafs(p, NULL);
			  /* Array with only pointers. */
			  while (leaf && curBytePosition < dataBytes) {
				  for (pointer cur = leaf; cur < leaf + UM_CHUNK_ARRAY_PAYLOAD_SIZE; cur += OBJPTR_SIZE) {
					  callIfIsObjptr(s, f, (objptr *) cur);
					  curBytePosition += OBJPTR_SIZE;
				  }
				  leaf = UM_walk_array_leafs(leaf, NULL);
			  }
		  } else {
			  /* Array with a mix of data where each element is arranged as:
			   *    ( (non heap-pointers)* :: (heap pointers)* )*
			   */
			  bool done = FALSE;
			  while (!done) {
				  size_t elem_size = bytesNonObjptrs + numObjptrs * OBJPTR_SIZE;
				  size_t array_len = pointerToArrayChunk(p)->array_chunk_length;
				  for (int i = 0; i < array_len; i++) {
					  pointer elptr = UM_Array_offset(s, p, i, elem_size, 0) + bytesNonObjptrs;
					  for (int j = 0; j < numObjptrs; j++) {
						  callIfIsObjptr(s, f, (objptr *) elptr);
						  elptr += OBJPTR_SIZE;
					  }
				  }
			  }
		  }
	  }
  } else { /* stack frame */
  	  // mark the objptrs inside of the given frame

	  assert (STACK_TAG == tag);
	  GC_UM_Chunk stackFrame = (GC_UM_Chunk)(p - GC_HEADER_SIZE);
	  GC_returnAddress returnAddress = *(uintptr_t*)(
	  		stackFrame->ml_object + stackFrame->ra);
	  assert (stackFrame->ra != 0); // cant process a frame if we dont know its layout

	  if (DEBUG_RTGC) {
		  fprintf (stderr, "%d] "YELLOW("frame")":  frame = "FMTPTR"  return address = "FMTRA" (%d) (ra=%d) "FMTPTR"\n",
				  PTHREAD_NUM,
				  (uintptr_t)stackFrame,
				  returnAddress,
				  returnAddress,
				  stackFrame->ra,
				  (uintptr_t)(
						  stackFrame->ml_object + stackFrame->ra + GC_HEADER_SIZE));
	  }

	  GC_frameLayout frameLayout = getFrameLayoutFromReturnAddress (s, returnAddress);
	  GC_frameOffsets frameOffsets = frameLayout->offsets;

	  if (DEBUG_RTGC)
		  fprintf(stderr, "%d]   frame: kind %s size %"PRIx16"\n",
			  PTHREAD_NUM, (frameLayout->kind==C_FRAME)?"C_FRAME":"ML_FRAME", frameLayout->size);

	  for (uint32_t i = 0 ; i < frameOffsets[0] ; ++i) {
		  uintptr_t x = (uintptr_t)(
		  		stackFrame->ml_object + frameOffsets[i + 1] + GC_HEADER_SIZE);
		  unsigned int xv = *(objptr*)x;

		  if (DEBUG_RTGC)
			  fprintf(stderr, "%d]    offset 0x%"PRIx16" (%d) stackaddress "FMTOBJPTR" objptr "FMTOBJPTR"\n",
				  PTHREAD_NUM,
				  frameOffsets[i + 1],
				  frameOffsets[i + 1],
				  x,
				  xv);

		  callIfIsObjptr(s, f, (objptr *)x);
	  }
  }
  return p;
}

/* foreachObjptrInRange (s, front, back, f, skipWeaks)
 *
 * Apply f to each pointer between front and *back, which should be a
 * contiguous sequence of objects, where front points at the beginning
 * of the first object and *back points just past the end of the last
 * object.  f may increase *back (for example, this is done by
 * forward).  foreachObjptrInRange returns a pointer to the end of
 * the last object it visits.
 *
 * If skipWeaks, then the object pointer in weak objects is skipped.
 */

pointer foreachObjptrInRange (GC_state s, pointer front, pointer *back,
                              GC_foreachObjptrFun f, bool skipWeaks) {
  pointer b;

  assert (isFrontierAligned (s, front));
  if (DEBUG_DETAILED)
    fprintf (stderr, 
             "%d] foreachObjptrInRange  front = "FMTPTR"  *back = "FMTPTR"\n",
             PTHREAD_NUM, (uintptr_t)front, (uintptr_t)(*back));
  b = *back;
  assert (front <= b);
  while (front < b) {
    while (front < b) {
      assert (isAligned ((size_t)front, GC_MODEL_MINALIGN));
      if (DEBUG_DETAILED)
        fprintf (stderr, 
                 "%d]  front = "FMTPTR"  *back = "FMTPTR"\n",
                 PTHREAD_NUM, (uintptr_t)front, (uintptr_t)(*back));
      pointer p = advanceToObjectData (s, front);
      assert (isAligned ((size_t)p, s->alignment));
      front = foreachObjptrInObject (s, p, f, skipWeaks);
    }
    b = *back;
  }
  return front;
}
