/* Copyright (C) 2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#define MLTON_GC_INTERNAL_TYPES
#include "platform.h"
#include "gc/um_constants.h"
struct GC_state gcState;
#define GENSIZES "/* gensizes */"

int mainX (__attribute__ ((unused)) int argc,
          __attribute__ ((unused)) char* argv[]) {
  FILE *sizesFd;

  sizesFd = stdout; // fopen_safe ("sizes", "w");

  fprintf (sizesFd, "cint = %"PRIuMAX" %s\n",     (uintmax_t)sizeof(C_Int_t), GENSIZES);
  fprintf (sizesFd, "cpointer = %"PRIuMAX" %s\n", (uintmax_t)sizeof(C_Pointer_t), GENSIZES);
  fprintf (sizesFd, "cptrdiff = %"PRIuMAX" %s\n", (uintmax_t)sizeof(C_Ptrdiff_t), GENSIZES);
  fprintf (sizesFd, "csize = %"PRIuMAX" %s\n",    (uintmax_t)sizeof(C_Size_t), GENSIZES);
  fprintf (sizesFd, "header = %"PRIuMAX" %s\n",   (uintmax_t)sizeof(GC_header), GENSIZES);
  fprintf (sizesFd, "mplimb = %"PRIuMAX" %s\n",   (uintmax_t)sizeof(C_MPLimb_t), GENSIZES);
  fprintf (sizesFd, "objptr = %"PRIuMAX" %s\n",   (uintmax_t)sizeof(objptr), GENSIZES);
  fprintf (sizesFd, "seqIndex = %"PRIuMAX" %s\n", (uintmax_t)sizeof(GC_arrayLength), GENSIZES);
  fprintf (sizesFd, "objChunkSize = %"PRIuMAX" %s\n", (uintmax_t)UM_CHUNK_PAYLOAD_SIZE, GENSIZES);

  fclose_safe(sizesFd);

  return 0;
}
