/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

__attribute__((__noreturn__)) size_t GC_size (GC_state s, pointer root) {

  die("Trying to do GC_size");
#if 0
  size_t res;
  
  enter (s); /* update stack in heap, in case it is reached */
  if (DEBUG_SIZE)
    fprintf (stderr, "GC_size marking\n");
  res = dfsMarkByMode (s, root, MARK_MODE, FALSE, FALSE);
  if (DEBUG_SIZE)
    fprintf (stderr, "GC_size unmarking\n");
  dfsMarkByMode (s, root, UNMARK_MODE, FALSE, FALSE);
  leave(s);
  
  return res;
#endif
}
