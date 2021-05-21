/* Copyright (C) 1999-2006 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

GC_frameIndex getFrameIndexFromReturnAddress (GC_state s, GC_returnAddress ra) {
  GC_frameIndex res;

  res = s->returnAddressToFrameIndex (ra);
  if (DEBUG_DETAILED)
    fprintf (stderr, "%d] "FMTFI" = getFrameIndexFromReturnAddress ("FMTRA")\n",
             PTHREAD_NUM, res, ra);
  return res;
}

GC_frameLayout getFrameLayoutFromFrameIndex (GC_state s, GC_frameIndex findex) {
  GC_frameLayout layout;

  if (DEBUG_DETAILED)
    fprintf (stderr, "%d] findex = "FMTFI"  frameLayoutsLength = %"PRIu32"\n",
            PTHREAD_NUM,findex, s->frameLayoutsLength);
  if (findex >= s->frameLayoutsLength) {
  	fprintf (stderr, "%d] findex %d >= %d s->frameLayoutsLength\n",
			PTHREAD_NUM, findex,
			s->frameLayoutsLength);
  }
  assert (findex < s->frameLayoutsLength);
  layout = &(s->frameLayouts[findex]);
  assert (layout->size > 0);

  // fprintf(stderr, "Frame size: %ld\n", layout->size);
  return layout;
}

GC_frameLayout getFrameLayoutFromReturnAddress (GC_state s, GC_returnAddress ra) {
  GC_frameLayout layout;
  GC_frameIndex findex;

  findex = getFrameIndexFromReturnAddress (s, ra);
  layout = getFrameLayoutFromFrameIndex(s, findex);
  return layout;
}
