/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void callIfIsObjptr (GC_state s, GC_foreachObjptrFun f, objptr *opp) {
    if (isObjptr (*opp)) {
        f (s, opp);
        return;
    }

    if (DEBUG_MEM)
        fprintf(stderr, "  callIfIsObjptr: Not objptr 0x%x\n", *opp);
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
  if (DEBUG_DETAILED)
    fprintf (stderr, "%d] foreachGlobal threads\n", PTHREAD_NUM);
  callIfIsObjptr (s, f, &s->callFromCHandlerThread);
  callIfIsObjptr (s, f, &s->currentThread[PTHREAD_NUM]);
  callIfIsObjptr (s, f, &s->savedThread[PTHREAD_NUM]);
  callIfIsObjptr (s, f, &s->signalHandlerThread[PTHREAD_NUM]);
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
      fprintf(stderr, "foreach object in 0x%x\n", (uintptr_t)p);
  }
  GC_header header;
  uint16_t bytesNonObjptrs;
  uint16_t numObjptrs;
  GC_objectTypeTag tag;

  header = getHeader (p);
  splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);
  if (DEBUG_DETAILED)
    fprintf (stderr,
             "foreachObjptrInObject ("FMTPTR")"
             "  header = "FMTHDR
             "  tag = %s"
             "  bytesNonObjptrs = %d"
             "  numObjptrs = %d\n",
             (uintptr_t)p, header, objectTypeTagToString (tag),
             bytesNonObjptrs, numObjptrs);
  if (NORMAL_TAG == tag) {
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
      if(p > s->heap.start && p< (s->heap.start+s->heap.size))
      {
        die("Non stack Object in old heap");
      }

      if (DEBUG_MEM)
          fprintf(stderr, "   foreachObjptrInObject, normal, bytesNonObjptrs: %d, "
                  "num ptrs: %d\n", bytesNonObjptrs, numObjptrs);

//      pointer p_base = p;
      //    p = p + bytesNonObjptrs;
//      pointer p1;
//      p += bytesNonObjptrs;
//      pointer p1 = p;

      for (int i=0; i<numObjptrs; i++) {
          pointer todo = UM_CPointer_offset(s, p, bytesNonObjptrs + i * OBJPTR_SIZE,
                                            OBJPTR_SIZE);
          /*
          p = UM_CPointer_offset(s, p_base,
                                 bytesNonObjptrs + i * OBJPTR_SIZE,
                                 OBJPTR_SIZE);


          if (DEBUG_MEM)
              fprintf(stderr, "   foreachObjptrInObject, normal, todo: 0x%x,"
                      " todoVal: "FMTOBJPTR"\n", (uintptr_t)p, (uintptr_t)
                      *((objptr*)p));
          */
//          if((objptr*)p1 != (objptr*)p) {
//              die("OBJ POINTER NOT EQUAL!!\n");
//          }
          callIfIsObjptr (s, f, (objptr*)todo);
//          p += OBJPTR_SIZE;
      }
//      p += bytesNonObjptrs;

  } else if (WEAK_TAG == tag) {
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
  }else { /* stack */
    GC_stack stack;
    pointer top, bottom;
    unsigned int i;
    GC_returnAddress returnAddress;
    GC_frameLayout frameLayout;
    GC_frameOffsets frameOffsets;

    assert (STACK_TAG == tag);
    stack = (GC_stack)p;
    bottom = getStackBottom (s, stack);
    top = getStackTop (s, stack);
    if(DEBUG_STACKS)
    fprintf(stderr,"%d] Checking Stack "FMTPTR" \n",PTHREAD_NUM,(uintptr_t)stack);
    /* we avoid checking the main thread's stack when the main calls into user code*/ 
    bool doit = true;
    if(s->mainBooted)
    {
        pointer p = objptrToPointer(s->currentThread[0], s->heap.start);
        GC_thread th = (GC_thread)(p + offsetofThread (s));

        GC_stack st = (GC_stack)objptrToPointer(th->stack, s->heap.start);
        
        if (st == stack)
            doit = false;

    }
    if(doit)
    {
    if (DEBUG_STACKS) {
      fprintf (stderr, "%d]  bottom = "FMTPTR"  top = "FMTPTR"\n",
               PTHREAD_NUM,
               (uintptr_t)bottom, (uintptr_t)top);
    }
    assert (stack->used <= stack->reserved);
    while (top > bottom) {
      /* Invariant: top points just past a "return address". */
      returnAddress = *((GC_returnAddress*)(top - GC_RETURNADDRESS_SIZE));
      if (DEBUG_STACKS) {
        fprintf (stderr, "%d]  top = "FMTPTR"  return address = "FMTRA"\n",
                 PTHREAD_NUM,
                 (uintptr_t)top, returnAddress);
      }
      frameLayout = getFrameLayoutFromReturnAddress (s, returnAddress);
      frameOffsets = frameLayout->offsets;
      top -= frameLayout->size;

      if (DEBUG_STACKS)
           fprintf(stderr, "%d]   frame: kind %s size %"PRIx16"\n",
                   PTHREAD_NUM, (frameLayout->kind==C_FRAME)?"C_FRAME":"ML_FRAME", frameLayout->size);

      for (i = 0 ; i < frameOffsets[0] ; ++i) {
        if (DEBUG_STACKS)
          fprintf(stderr, "%d]    offset %"PRIx16"  address "FMTOBJPTR"\n",
                  PTHREAD_NUM,
                  frameOffsets[i + 1], 
                  *(objptr*)(top + frameOffsets[i + 1]));
        callIfIsObjptr (s, f, (objptr*)(top + frameOffsets[i + 1]));
      }
    }
    assert(top == bottom);
    }
    p += sizeof (struct GC_stack) + stack->reserved;
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
  pointer bottom;
  GC_frameIndex findex;
  GC_frameLayout layout;
  GC_returnAddress returnAddress;
  pointer top;

  if (DEBUG_PROFILE)
    fprintf (stderr, "%d] foreachStackFrame\n", PTHREAD_NUM);
  bottom = getStackBottom (s, getStackCurrent(s));
  if (DEBUG_PROFILE)
    fprintf (stderr, "%d]  bottom = "FMTPTR"  top = "FMTPTR".\n",
             PTHREAD_NUM,
             (uintptr_t)bottom, (uintptr_t)s->stackTop[PTHREAD_NUM]);
  for (top = s->stackTop[PTHREAD_NUM]; top > bottom; top -= layout->size) {
    returnAddress = *((GC_returnAddress*)(top - GC_RETURNADDRESS_SIZE));
    findex = getFrameIndexFromReturnAddress (s, returnAddress);
    if (DEBUG_PROFILE)
      fprintf (stderr, "%d] top = "FMTPTR"  findex = "FMTFI"\n",
               PTHREAD_NUM,
               (uintptr_t)top, findex);
    unless (findex < s->frameLayoutsLength)
      die ("%d] top = "FMTPTR"  returnAddress = "FMTRA"  findex = "FMTFI"\n",
           PTHREAD_NUM,
           (uintptr_t)top, (uintptr_t)returnAddress, findex);
    f (s, findex);
    layout = &(s->frameLayouts[findex]);
    assert (layout->size > 0);
  }
  if (DEBUG_PROFILE)
    fprintf (stderr, "%d] done foreachStackFrame\n", PTHREAD_NUM);
}
