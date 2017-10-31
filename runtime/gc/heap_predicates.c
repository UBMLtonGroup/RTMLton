/* Copyright (C) 2012 Matthew Fluet.
 * Copyright (C) 2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void displayHeapInfo(GC_state s) {
	fprintf(stderr, "start "FMTPTR" -> from-start "FMTPTR"  -> nursery "FMTPTR" -> frontier "FMTPTR"\n",
			s->heap.start, s->heap.start+s->heap.oldGenSize,
			s->heap.nursery, s->frontier);
}

bool isPointerInOldGen (GC_state s, pointer p) {
  return (not (isPointer (p))
          or (s->heap.start <= p 
              and p < s->heap.start + s->heap.oldGenSize));
}

bool isPointerInNursery (GC_state s, pointer p) {
  return (not (isPointer (p))
          or (s->heap.nursery <= p and p < s->frontier));
}

#if ASSERT
bool isObjptrInOldGen (GC_state s, objptr op) {
  pointer p;
  if (not (isObjptr(op)))
    return TRUE;
  p = objptrToPointer (op, s->heap.start);
  return isPointerInOldGen (s, p);
}
#endif

bool isObjptrInNursery (GC_state s, objptr op) {
  pointer p;
  if (not (isObjptr(op)))
    return TRUE;
  p = objptrToPointer (op, s->heap.start);
  return isPointerInNursery (s, p);
}

#if ASSERT
bool isObjptrInFromSpace (GC_state s, objptr op) {
  if (0 && DEBUG) {
	fprintf(stderr, "%d] isObjprtInFromSpace: isObjptr:%x op:%x p:%x\n",PTHREAD_NUM,
			isObjptr(op), op, isObjptr(op)? objptrToPointer (op, s->heap.start) : 0);
  }

  return (isObjptrInOldGen (s, op) 
          or isObjptrInNursery (s, op));
}
#endif

bool hasHeapBytesFree (GC_state s, size_t oldGen, size_t nursery) {
  size_t total;
  bool res;

  if (DEBUG_DETAILED) {
      displayHeap(s, &(s->heap), stderr);
      displayHeapInfo(s);
      fprintf(stderr,"%d]in hasHeapBytesFree. OldGenSize = %zu\n", PTHREAD_NUM,s->heap.oldGenSize);
  }

  total =
    s->heap.oldGenSize + oldGen 
    + (s->canMinor ? 2 : 1) * (size_t)(s->limitPlusSlop - s->heap.nursery);

  if(DEBUG_DETAILED) fprintf(stderr,"%d] total = %zu\n", PTHREAD_NUM,total);

  res = 
    (total <= s->heap.size) 
    and (nursery <= (size_t)(s->limitPlusSlop - s->frontier));

  if (DEBUG_DETAILED)
    fprintf (stderr, "%s = hasBytesFree (%s, %s)\n",
             boolToString (res),
             uintmaxToCommaString(oldGen),
             uintmaxToCommaString(nursery));
  return res;
}

bool isHeapInit (GC_heap h) {
  return (0 == h->size);
}
