/* Copyright (C) 1999-2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#define CHECKDISABLEGC do { if (getenv("DISABLEGC")) { fprintf(stderr, "GC is disabled\n"); return; } } while(0)

/* enter and leave should be called at the start and end of every GC
 * function that is exported to the outside world.  They make sure
 * that the function is run in a critical section and check the GC
 * invariant.
 */
void enter (GC_state s) {
  CHECKDISABLEGC;

  if (DEBUG_OLD)
    fprintf (stderr, "%d] enter\n", PTHREAD_NUM);
  /* used needs to be set because the mutator has changed s->stackTop. */
  //getStackCurrent(s)->used = sizeofGCStateCurrentStackUsed (s);

  /* these are objptrs, we adjust in c-chunk.h to get the actual point to the start of frame */
  getThreadCurrent(s)->currentFrame = s->currentFrame[PTHREAD_NUM];

  getThreadCurrent(s)->exnStack = s->exnStack[PTHREAD_NUM];

  if (DEBUG_OLD) 
    displayGCState (s, stderr);
  beginAtomic (s);
  assert (invariantForGC (s));
  if (DEBUG_OLD)
    fprintf (stderr, "%d] enter ok\n", PTHREAD_NUM);
}

void leave (GC_state s) {
  CHECKDISABLEGC;

  if (DEBUG_OLD)
    fprintf (stderr, "%d] leave\n", PTHREAD_NUM);
  /* The mutator frontier invariant may not hold
   * for functions that don't ensureBytesFree.
   */
  assert (invariantForMutator (s, FALSE, TRUE));
  endAtomic (s);
  if (DEBUG_OLD)
    fprintf (stderr, "%d] leave ok\n", PTHREAD_NUM);
}
