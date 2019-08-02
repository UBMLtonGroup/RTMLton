/*
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#if (defined (MLTON_GC_INTERNAL_TYPES))



#endif /* (defined (MLTON_GC_INTERNAL_TYPES)) */

#if (defined (MLTON_GC_INTERNAL_FUNCS))

static void um_displayStack (GC_state s, GC_stack stack, FILE *stream);

static inline bool um_isStackEmpty (GC_stack stack);

static inline pointer um_getStackBottom (GC_state s, objptr stack);
static inline pointer um_getStackTop (GC_state s, objptr stack);

static inline void um_copyStack (GC_state s, GC_thread from, GC_thread to);

static inline size_t um_offsetofStack (GC_state s);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
