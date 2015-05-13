/* Copyright (C) 2014 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _COMMON_MAIN_H_
#define _COMMON_MAIN_H_

#include "mlton-main.h"

#define MLTON_GC_INTERNAL_TYPES
#define MLTON_GC_INTERNAL_BASIS
#include "platform.h"

/* The label must be declared as weak because gcc's optimizer may prove that
 * the code that declares the label is dead and hence eliminate the declaration.
 */
#define DeclareProfileLabel(l)                  \
        extern char l __attribute__ ((weak))

#define BeginVectorInits static struct GC_vectorInit vectorInits[] = {
#define VectorInitElem(es, gi, l, w) { es, gi, l, w },
#define EndVectorInits };

#define LoadArray(a, f) if (fread (a, sizeof(*a), cardof(a), f) != cardof(a)) return -1;
#define SaveArray(a, f) if (fwrite(a, sizeof(*a), cardof(a), f) != cardof(a)) return -1;

PRIVATE Pointer gcStateAddress;

#define Initialize(al, mg, mfs, mmc, pk, ps)                            \
        gcStateAddress = (pointer)&gcState;                             \
        gcState.alignment = al;                                         \
        gcState.atMLtons = atMLtons;                                    \
        gcState.atMLtonsLength = cardof(atMLtons);                      \
        gcState.frameLayouts = frameLayouts;                            \
        gcState.frameLayoutsLength = cardof(frameLayouts);              \
        gcState.globals = (objptr*)globalObjptr;                        \
        gcState.globalsLength = cardof(globalObjptr);                   \
        gcState.loadGlobals = loadGlobals;                              \
        gcState.magic = mg;                                             \
        gcState.maxFrameSize = mfs;                                     \
        gcState.mutatorMarksCards = mmc;                                \
        gcState.objectTypes = objectTypes;                              \
        gcState.objectTypesLength = cardof(objectTypes);                \
        gcState.returnAddressToFrameIndex = returnAddressToFrameIndex;  \
        gcState.saveGlobals = saveGlobals;                              \
        gcState.vectorInits = vectorInits;                              \
        gcState.vectorInitsLength = cardof(vectorInits);                \
        gcState.sourceMaps.frameSources = frameSources;                 \
        gcState.sourceMaps.frameSourcesLength = cardof(frameSources);   \
        gcState.sourceMaps.sourceLabels = sourceLabels;                 \
        gcState.sourceMaps.sourceLabelsLength = cardof(sourceLabels);   \
        gcState.sourceMaps.sourceNames = sourceNames;                   \
        gcState.sourceMaps.sourceNamesLength = cardof(sourceNames);     \
        gcState.sourceMaps.sourceSeqs = sourceSeqs;                     \
        gcState.sourceMaps.sourceSeqsLength = cardof(sourceSeqs);       \
        gcState.sourceMaps.sources = sources;                           \
        gcState.sourceMaps.sourcesLength = cardof(sources);             \
        gcState.profiling.kind = pk;                                    \
        gcState.profiling.stack = ps;                                   \
        gcState.GCrunnerRunning = FALSE;                                \
        MLton_init (argc, argv, &gcState);                              \
                                                                        \
        unsigned int NUM_REALTIME_THREADS = 2; /*disabled*/                          \
        pthread_t *realtimeThreads =                                    \
                malloc(NUM_REALTIME_THREADS * sizeof(pthread_t));       \
        assert(realtimeThreads != NULL);                                \
		pthread_t *GCrunner_thread = malloc(sizeof(pthread_t));         \
		assert(GCrunner_thread != NULL);                                \
		assert(pthread_mutux_init(&(gcState.gc_mutex), NULL) == 0);             \
		assert(pthread_mutex_lock(&(gcState.gc_mutex)) == 0);             \
		fprintf(stderr, "%x] main thread locking %x\n", pthread_self(), gcStateAddress); \
		assert(pthread_mutex_trylock(&(gcState.gc_mutex)) == EBUSY);      \
		fprintf(stderr, "relock\n"); pthread_mutex_lock(&gcState.gc_mutex); pthread_create(GCrunner_thread, NULL, &GCrunner, (void*)&gcState); \
        while (!gcState.GCrunnerRunning){}; \
          fprintf(stderr, "sleeping 10\n"); sleep(10);                                                              \
        unsigned int tNum;                                              \
        for (tNum = 2; tNum < NUM_REALTIME_THREADS; tNum++) {           \
        	fprintf(stderr, "spawning thread %d\n", tNum);              \
            if (pthread_create(&realtimeThreads[tNum], NULL, NULL, \
                        (void*)&gcState)) {                             \
                fprintf (stderr, "pthread_create failed: %s\n", strerror (errno)); \
                exit (1);                                               \
            }                                                           \
        }                                                               \
                                                                        \
        gcState.numRealtimeThreads = NUM_REALTIME_THREADS;              \
        gcState.realtimeThreads = realtimeThreads;                      \
                                                                        \
        bool *rtAllocated = malloc(NUM_REALTIME_THREADS * sizeof(bool));\
        for (tNum = 0; tNum < NUM_REALTIME_THREADS; tNum++) {           \
            rtAllocated[tNum] = false;                                  \
        }                                                               \
                                                                        \
        gcState.realtimeThreadAllocated = rtAllocated;                  \

#define LIB_PASTE(x,y) x ## y
#define LIB_OPEN(x) LIB_PASTE(x, _open)
#define LIB_CLOSE(x) LIB_PASTE(x, _close)

static void MLton_callFromC ();

#endif /* #ifndef _COMMON_MAIN_H_ */
