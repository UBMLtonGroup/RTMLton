/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _C_MAIN_H_
#define _C_MAIN_H_

#include "common-main.h"
#include "c-common.h"


#ifndef PTHREAD_NUM
# define PTHREAD_NUM get_pthread_num()
#endif

static GC_frameIndex returnAddressToFrameIndex (GC_returnAddress ra) {
        return (GC_frameIndex)ra;
}

#define MLtonCallFromC                                                  \
/* Globals */                                                           \
PRIVATE uintptr_t YYZnextFun[MAXPRI];                                      \
PRIVATE int returnToC[MAXPRI];                                          \
static void MLton_callFromC () {                                        \
		fprintf(stderr, "%d] c-main MLton_callFromC\n", PTHREAD_NUM);   \
        struct cont cont;                                               \
        GC_state s;                                                     \
                                                                        \
        if (DEBUG_CCODEGEN)                                             \
                fprintf (stderr, "MLton_callFromC() starting\n");       \
        s = &gcState;                                                   \
        GC_setSavedThread (s, GC_getCurrentThread (s));                 \
        s->atomicState += 3;                                            \
        if (s->signalsInfo.signalIsPending)                             \
                s->limit = s->limitPlusSlop - GC_HEAP_LIMIT_SLOP;       \
        /* Switch to the C Handler thread. */                           \
        GC_switchToThread (s, GC_getCallFromCHandlerThread (s), 0);     \
        cont.nextFun =                                                  \
        		*(uintptr_t*)(s->stackTop[PTHREAD_NUM] - GC_RETURNADDRESS_SIZE);   \
        cont.nextChunk = nextChunks[cont.nextFun];                      \
        returnToC[PTHREAD_NUM] = FALSE;                                 \
        fprintf(stderr, "%d] go to C->SML call %x\n", PTHREAD_NUM, s);  \
        do {                                                            \
                cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
        } while (not returnToC[PTHREAD_NUM]);                           \
        returnToC[PTHREAD_NUM] = FALSE;                                 \
        s->atomicState += 1;                                            \
        fprintf(stderr, "%d] back from C->SML call\n", PTHREAD_NUM);    \
        GC_switchToThread (s, GC_getSavedThread (s), 0);                \
        s->atomicState -= 1;                                            \
        if (0 == s->atomicState                                         \
            && s->signalsInfo.signalIsPending)                          \
                s->limit = 0;                                           \
        if (DEBUG_CCODEGEN)                                             \
                fprintf (stderr, "MLton_callFromC done\n");             \
}


#define MLtonMain(al, mg, mfs, mmc, pk, ps, mc, ml)                     \
extern pthread_mutex_t gclock;                                          \
MLtonCallFromC                                                          \
PUBLIC int MLton_main (int argc, char* argv[]) {                        \
        struct cont cont;                                               \
        Initialize (al, mg, mfs, mmc, pk, ps);                          \
        if (gcState.amOriginal) {                                       \
                real_Init();                                            \
                PrepFarJump(cont, mc, ml);                                    \
        } else {                                                        \
                /* Return to the saved world */                         \
                cont.nextFun = *(uintptr_t*)(gcState.stackTop[PTHREAD_NUM] - GC_RETURNADDRESS_SIZE); \
                cont.nextChunk = nextChunks[cont.nextFun];                   \
        }                                                               \
        setvbuf(stderr, NULL, _IONBF, 0);                               \
	pthread_t *GCrunner_thread = malloc(sizeof(pthread_t));         \
	assert(GCrunner_thread != NULL);                                \
	MYASSERT(pthread_mutex_init(&gclock, NULL), ==, 0);             \
	MYASSERT(pthread_mutex_lock(&gclock), ==, 0);                   \
	DBG((stderr, "%x] main thread locking %x\n", pthread_self(), &gclock));             \
	MYASSERT(pthread_create(GCrunner_thread, NULL, &GCrunner, (void*)&gcState), ==, 0); \
    	DBG((stderr, "%d] waiting on GC runner", PTHREAD_NUM));         \
	while (!gcState.GCrunnerRunning){DBG((stderr, "spin.."));}      \
	realtimeThreadInit(&gcState);					\
       									\
        /* Trampoline */                                                \
		while (1) {                                                     \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
				cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
		}                                                               \
        return 1;                                                       \
}

#define MLtonLibrary(al, mg, mfs, mmc, pk, ps, mc, ml)                  \
MLtonCallFromC                                                          \
PUBLIC void LIB_OPEN(LIBNAME) (int argc, char* argv[]) {                \
        struct cont cont;                                               \
        Initialize (al, mg, mfs, mmc, pk, ps);                          \
        if (gcState.amOriginal) {                                       \
                real_Init();                                            \
                PrepFarJump(cont, mc, ml);                                    \
        } else {                                                        \
                /* Return to the saved world */                         \
                cont.nextFun = *(uintptr_t*)(gcState.stackTop[PTHREAD_NUM] - GC_RETURNADDRESS_SIZE); \
                cont.nextChunk = nextChunks[cont.nextFun];                   \
        }                                                               \
        /* Trampoline */                                                \
        returnToC[PTHREAD_NUM] = FALSE;                                              \
        do {                                                            \
                cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
        } while (not returnToC[PTHREAD_NUM]);                                        \
}                                                                       \
PUBLIC void LIB_CLOSE(LIBNAME) () {                                     \
        struct cont cont;                                               \
        cont.nextFun = *(uintptr_t*)(gcState.stackTop[PTHREAD_NUM] - GC_RETURNADDRESS_SIZE); \
        cont.nextChunk = nextChunks[cont.nextFun];                           \
        returnToC[PTHREAD_NUM] = FALSE;                                              \
        do {                                                            \
                cont=(*(struct cont(*)(uintptr_t))cont.nextChunk)(cont.nextFun);         \
        } while (not returnToC)[PTHREAD_NUM];                                        \
        GC_done(&gcState);                                              \
}

#endif /* #ifndef _C_MAIN_H */
