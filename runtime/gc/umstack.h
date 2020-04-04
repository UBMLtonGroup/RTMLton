/*
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#if (defined (MLTON_GC_INTERNAL_TYPES))



#endif /* (defined (MLTON_GC_INTERNAL_TYPES)) */

void um_dumpStack (GC_state s);

#if (defined (MLTON_GC_INTERNAL_FUNCS))
void um_dumpFrame (GC_state s, objptr chunk, GC_returnAddress raoverride);

static void um_displayStack (GC_state s, GC_stack stack, FILE *stream) __attribute__ ((unused));

static inline bool um_isStackEmpty (GC_stack stack);

static inline pointer um_getStackBottom (GC_state s, objptr stack);
static inline pointer um_getStackTop (GC_state s, objptr stack);

static inline void um_copyStack (GC_state s, GC_thread from, GC_thread to);

static inline size_t um_offsetofStack (GC_state s);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
