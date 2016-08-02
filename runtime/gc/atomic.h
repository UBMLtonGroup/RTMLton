/* Copyright (C) 1999-2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#if (defined (MLTON_GC_INTERNAL_FUNCS))

static inline void beginAtomic (GC_state s);
static inline void endAtomic (GC_state s);
static inline void setAtomic (GC_state s, uint32_t v);
static inline void decAtomicBy (GC_state s, uint32_t v);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */

inline void incAtomic (GC_state s);
inline void decAtomic (GC_state s);
inline void incAtomicBy (GC_state s, uint32_t v);



