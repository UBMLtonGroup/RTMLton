/* Copyright (C) 2009 Matthew Fluet.
 * Copyright (C) 1999-2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void *GCrunner(void *);
extern pthread_mutex_t gcflag_lock;

#if (defined (MLTON_GC_INTERNAL_FUNCS))

static void minorGC (GC_state s);
static inline void growStackCurrent (GC_state s);
static inline void enterGC (GC_state s);
static inline void leaveGC (GC_state s);

static void performUMGC(GC_state s,
                        size_t ensureObjectChunksAvailable,
                        size_t ensureArrayChunksAvailable,
                        bool fullGC);



#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))

PRIVATE void GC_collect (GC_state s, size_t bytesRequested, bool force);
void maybe_growstack(GC_state s);

#endif /* (defined (MLTON_GC_INTERNAL_BASIS)) */
