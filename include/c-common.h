/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _C_COMMON_H_
#define _C_COMMON_H_

#define MAXPRI 2 /* XXX dup'd in runtime/realtime_thread.h */
#ifndef DEBUG_CCODEGEN
#define DEBUG_CCODEGEN FALSE
#endif

#include "export.h"

struct cont {
        void *nextChunk;
		uintptr_t nextFun;
};

PRIVATE extern uintptr_t XXXnextFun[MAXPRI];
PRIVATE extern int returnToC[MAXPRI];
PRIVATE extern struct cont (*nextChunks []) (void);

#define ChunkName(n) Chunk ## n

#define DeclareChunk(n)                         \
        PRIVATE struct cont ChunkName(n)(uintptr_t l_nextFun)

#define Chunkp(n) &(ChunkName(n))

#define PrepFarJump(cont, n, l)                               \
        do {                                            \
                cont.nextChunk = (void*)ChunkName(n);   \
                cont.nextFun = l;                       \
        } while (0)

#endif /* #ifndef _C_COMMON_H_ */
