/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#define MLTON_UTIL
#if defined(__rtems__)
#include "/opt/rtems6/i386-rtems6/include/stdio.h"
#else
#pragma message "not rtems"
#endif

#include "util/die.c"
#include "util/to-string.c"
