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
static void majorGC (GC_state s, size_t bytesRequested, bool mayResize);
static inline void growStackCurrent (GC_state s);
static inline void enterGC (GC_state s);
static inline void leaveGC (GC_state s);
static void performGC (GC_state s,
                       size_t oldGenBytesRequested,
                       size_t nurseryBytesRequested,
                       bool forceMajor,
                       bool mayResize);
static void performUMGC(GC_state s,
                        size_t ensureObjectChunksAvailable,
                        size_t ensureArrayChunksAvailable,
                        bool fullGC);
static void performGC_helper (GC_state s,
								size_t oldGenBytesRequested,
								size_t nurseryBytesRequested,
								bool forceMajor,
								bool mayResize);

static void markStack(GC_state s, GC_stack currentStack);
static void startMarking(GC_state s);

static void sweep(GC_state s, size_t ensureObjectChunksAvailable,size_t ensureArrayChunksAvailable,
        bool fullGC);

static inline void ensureInvariantForMutator (GC_state s, bool force);
bool ensureChunksAvailable(GC_state s);
void GC_collect_real(GC_state s, size_t bytesREquested,bool force);
static inline void ensureHasHeapBytesFree (GC_state s,
                                           size_t oldGenBytesRequested,
                                           size_t nurseryBytesRequested);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

#if (defined (MLTON_GC_INTERNAL_BASIS))
PRIVATE void dummyCFunc (void);
PRIVATE void GC_collect (GC_state s, size_t bytesRequested, bool force);
void maybe_growstack(GC_state s);

#endif /* (defined (MLTON_GC_INTERNAL_BASIS)) */
