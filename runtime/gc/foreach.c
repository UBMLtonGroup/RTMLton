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
	//fprintf(stderr, "callIfIsObjptr "FMTPTR"\n", (unsigned int)opp);
    if (isObjptr (*opp)) {
        f (s, opp);
        return;
    }

    if (DEBUG_MEM)
        fprintf(stderr, "  callIfIsObjptr: Not objptr 0x%x\n", *opp);
}


void foreachGlobalThreadObjptr(GC_state s, GC_foreachObjptrFun f)
{

  //if (DEBUG_DETAILED)
  //  fprintf (stderr, "%d] foreachGlobal threads\n", PTHREAD_NUM);


  // in rtmlton i think it is safe to disable this feature
  // where you can call SML /from/ C. our research is focused (?) on
  // pure SML systems.

  fprintf (stderr, "%d] callFromCHandlerThread\n", PTHREAD_NUM);
  callIfIsObjptr (s, f, &s->callFromCHandlerThread);

  fprintf (stderr, "%d] currentThread\n", PTHREAD_NUM);
  callIfIsObjptr (s, f, &s->currentThread[PTHREAD_NUM]);

  fprintf (stderr, "%d] savedThread\n", PTHREAD_NUM);
  callIfIsObjptr (s, f, &s->savedThread[PTHREAD_NUM]);

  fprintf (stderr, "%d] signalHandlerThread\n", PTHREAD_NUM);
  callIfIsObjptr (s, f, &s->signalHandlerThread[PTHREAD_NUM]);
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
  uint16_t bytesNonObjptrs =0;
  uint16_t numObjptrs =0;
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

  	/*
      p += bytesNonObjptrs;

      pointer max = p + (numObjptrs * OBJPTR_SIZE);

      for ( ; p < max; p += OBJPTR_SIZE) {
          fprintf (stderr,
                   "Should have:  p = "FMTPTR"  *p = "FMTOBJPTR"\n",
                   (uintptr_t)p, *(objptr*)p);
          callIfIsObjptr (s, f, (objptr*)p);
      }
*/
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

		if (((unsigned int )(*todo)) == 0) {
			fprintf(stderr, "TODO is null\n");
			um_dumpStack(s);
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

	  GC_UM_Array_Chunk fst_leaf = (GC_UM_Array_Chunk)(p - GC_HEADER_SIZE - GC_HEADER_SIZE);
      if (fst_leaf->array_chunk_length > 0) {
          size_t length = fst_leaf->array_chunk_length;
          GC_UM_Array_Chunk cur_chunk = fst_leaf;
          size_t i, j;
          size_t elem_size = bytesNonObjptrs + numObjptrs * OBJPTR_SIZE;
          for (i=0; i<length; i++) {
              pointer start = (pointer)&(cur_chunk->ml_array_payload.ml_object[0]);
              size_t offset = (i % fst_leaf->array_chunk_numObjs) * elem_size + bytesNonObjptrs;
              pointer pobj = start + offset;
              for (j=0; j<numObjptrs; j++) {
                  callIfIsObjptr (s, f, (objptr*)pobj);
                  pobj += OBJPTR_SIZE;
              }

              if (i > 0 && i % fst_leaf->array_chunk_numObjs == 0)
                  cur_chunk = cur_chunk->next_chunk;
          }
      }
  } else { /* stack */
    GC_UM_Chunk stackFrame;
    unsigned int i;
    GC_returnAddress returnAddress;
    GC_frameLayout frameLayout;
    GC_frameOffsets frameOffsets;
	GC_thread thread = (GC_thread)s->currentThread[PTHREAD_NUM];

    fprintf(stderr, "%d] "GREEN("marking stack")"(foreachObjptrInObject)\n", PTHREAD_NUM);
    assert (STACK_TAG == tag);
    stackFrame = (GC_UM_Chunk)p;
	assert (stackFrame->next_chunk != NULL); // we will be starting at the chunk just after currentFrame

    if (DEBUG_STACKS)
    	fprintf(stderr,"%d] Checking Stack "FMTPTR" \n", PTHREAD_NUM, (uintptr_t)stackFrame);

    displayThread(s, thread, stderr);

	GC_UM_Chunk top = stackFrame;
	GC_UM_Chunk bottom =(GC_UM_Chunk) thread->firstFrame;;

	int counter = 0;
	int depth = 0;

	// count the depth of the current stack
	for (GC_UM_Chunk t = top ; t ; t = t->prev_chunk, depth++);

	assert (bottom != NULL);

	// mark all of this stack's chunks
	for( ; bottom->next_chunk ; bottom = bottom->next_chunk) {
		markChunk((pointer) & (bottom->ml_object), tag, MARK_MODE, s, 0);
	}

	fprintf(stderr, "%d] stack depth is %d\n", PTHREAD_NUM, depth);

	while (top->prev_chunk) {
		top = top->prev_chunk;

		//assert (top->ra != 0);
		returnAddress = *(uintptr_t*)(top->ml_object+top->ra);

		if (DEBUG_STACKS) {
			fprintf (stderr, "%d] frame %d:  top = "FMTPTR"  return address = "FMTRA" (%d) (ra=%d)\n",
					 PTHREAD_NUM, depth-counter,
					 (uintptr_t)top, returnAddress, returnAddress,
					 top->ra);
		}
		frameLayout = getFrameLayoutFromReturnAddress (s, returnAddress);
		frameOffsets = frameLayout->offsets;
		counter++;

		if (DEBUG_STACKS)
		   fprintf(stderr, "%d]   frame: kind %s size %"PRIx16"\n",
				   PTHREAD_NUM, (frameLayout->kind==C_FRAME)?"C_FRAME":"ML_FRAME", frameLayout->size);

		for (i = 0 ; i < frameOffsets[0] ; ++i) {
			uintptr_t x = (uintptr_t)(top->ml_object + frameOffsets[i + 1] + s->alignment);
			unsigned int xv = *(objptr*)x;

			if (DEBUG_STACKS)
				fprintf(stderr, "%d]    offset 0x%"PRIx16" (%d) stackaddress "FMTOBJPTR" objptr "FMTOBJPTR"\n",
					  PTHREAD_NUM,
					  frameOffsets[i + 1], frameOffsets[i + 1],
					  x,
					  xv);

			callIfIsObjptr(s, f, (objptr * )x);
		}
    }
    fprintf(stderr, "%d] done checking stack\n", PTHREAD_NUM);
    p = (pointer) stackFrame->next_chunk;
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


/* Apply f to the frame index of each frame in the current thread's stack. */
void foreachStackFrame (GC_state s, GC_foreachStackFrameFun f) {
  if (DEBUG_PROFILE)
    fprintf (stderr, "%d] foreachStackFrame\n", PTHREAD_NUM);

  if (DEBUG_PROFILE)
    fprintf (stderr, "%d] done foreachStackFrame\n", PTHREAD_NUM);
}
