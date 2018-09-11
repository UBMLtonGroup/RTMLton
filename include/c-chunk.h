/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _C_CHUNK_H_
#define _C_CHUNK_H_

#include <stdio.h>

#ifndef PTHREAD_NUM
# define PTHREAD_NUM get_pthread_num()
#endif

#include "ml-types.h"
#include "c-types.h"
#include "c-common.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef DEBUG_CCODEGEN
#define DEBUG_CCODEGEN FALSE
#endif

#define WORDWIDTH 8 /* use gcState->alignment */

#define NO_CACHE_STACK
#define NO_CACHE_FRONTIER

#define GCState ((Pointer)&gcState)
#define ExnStack *(size_t*)(GCState + ExnStackOffset+(PTHREAD_NUM*WORDWIDTH) )
#define FrontierMem *(Pointer*)(GCState + FrontierOffset)
#define UMFrontierMem *(Pointer*)(GCState + UMFrontierOffset)
#define Frontier *(Pointer*)(GCState + FrontierOffset)
// frontier
#define UMFrontier *(Pointer*)(GCState + UMFrontierOffset)
// umfrontier

#define StackBottom (*(Pointer*)(GCState + StackBottomOffset+(PTHREAD_NUM*WORDWIDTH)))
#define StackTopMem (*(Pointer*)(GCState + StackTopOffset+(PTHREAD_NUM*WORDWIDTH)))


#define UMStackBottom (*(Pointer*)(GCState + UMStackBottomOffset+(PTHREAD_NUM*WORDWIDTH)))
#define UMStackTopMem (*(Pointer*)(GCState + UMStackTopOffset+(PTHREAD_NUM*WORDWIDTH)))
#define CurrentFrame (*(Pointer*)(GCState + CurrentFrameOffset+(PTHREAD_NUM*WORDWIDTH)))

#define StackTop StackTopMem
#define UMStackTop UMStackTopMem


/* ------------------------------------------------- */
/*                      Memory                       */
/* ------------------------------------------------- */


#define C(ty, x) (*(ty*)(x))
#define G(ty, i) (global##ty [i])
#define GPNR(i) G(ObjptrNonRoot, i)
#undef DEBUG_MEMORY
#if 0

    #define O(ty, b, o) (*((fprintf (stderr, "%s:%d O: Addr=%018p Val=%018p\n", __FILE__, __LINE__, \
                                           (void*)((b) + (o)), \
                                           *((ty*)((b) + (o))))), \
                                 ((ty*)((b) + (o)))))

    #define X(ty, b, i, s, o) (*((fprintf (stderr, "%s:%d X: Addr=%018p Val=%018p\n", __FILE__, __LINE__, \
                                                 (void*)((b) + ((i) * (s)) + (o)), \
                                                 *(ty*)((b) + ((i) * (s)) + (o)))), \
                                       ((ty*)((b) + ((i) * (s)) + (o)))))

    #define S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: StackTop=%018p Addr=%018p Val=%018p\n", __FILE__, __LINE__,PTHREAD_NUM, \
                                (void*) StackTop, \
                               (void*)(StackTop + (i)), \
                               *(ty*)(StackTop + (i)))) , \
                        (ty*)(StackTop + (i))))



#endif

#define UMSTACKS

static int junk;

#ifdef UMSTACKS
#define O(ty, b, o) (*(ty*)((b) + (o)))
#define X(ty, gc_stat, b, i, s, o) (*(ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o))))
#define CHOFF(gc_stat, ty, b, o, s) (*(ty*)(UM_Chunk_Next_offset((gc_stat), (b), (o), (s))))

#define S2(ty, i) junk

#define S3(ty, i) (*((fprintf (stderr, "%s:%d %d] S: CurrentFrame=%018p Frame+Offset=%018p CurrentVal=%018p\n", \
                                 __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*)CurrentFrame, \
                                (void *)(CurrentFrame + (i)), \
                               *(ty*)(CurrentFrame + (i)))) , \
                                (ty*)(CurrentFrame + (i))))

#define S(ty, i) *(ty*)(CurrentFrame + (i))

#else

#define O(ty, b, o) (*(ty*)((b) + (o)))
// #define X(ty, b, i, s, o) (*(ty*)((b) + ((i) * (s)) + (o)))
#define X(ty, gc_stat, b, i, s, o) (*(ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o))))
#define S_temp_disabled(ty, i) *(ty*)(StackTop + (i))
#define CHOFF(gc_stat, ty, b, o, s) (*(ty*)(UM_Chunk_Next_offset((gc_stat), (b), (o), (s))))

#define S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: StackTop=%018p Addr=%018p Val=%018p\n", \
                                 __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*) StackTop, \
                                (void*)(StackTop + (i)),  \
                               *(ty*)(StackTop + (i)))),  \
                                (ty*)(StackTop + (i))))

#define S2(ty, i) junk

#endif

/* ------------------------------------------------- */
/*                       Tests                       */
/* ------------------------------------------------- */

#define IsInt(p) (0x3 & (int)(p))

#define BZ(x, l)                                                        \
        do {                                                            \
                if (DEBUG_CCODEGEN)                                     \
                        fprintf (stderr, "%s:%d: BZ(%d, %s)\n", \
                                        __FILE__, __LINE__, (x), #l);   \
                if (0 == (x)) goto l;                                   \
        } while (0)

#define BNZ(x, l)                                                       \
        do {                                                            \
                if (DEBUG_CCODEGEN)                                     \
                        fprintf (stderr, "%s:%d: BNZ(%d, %s)\n",        \
                                        __FILE__, __LINE__, (x), #l);   \
                if (x) goto l;                                          \
        } while (0)

#ifndef NO_CACHE_FRONTIER
#define FlushFrontier()                         \
        do {                                    \
                /* FrontierMem = Frontier; */     \
               /* UMFrontierMem = UMFrontier; */     \
        } while (0)
#else
#define FlushFrontier()
#endif

#ifndef NO_CACHE_STACK
#define FlushStackTop()                         \
        do {                                    \
                /* StackTopMem = StackTop; */        \
        } while (0)
#else
#define FlushStackTop() 
#endif

#ifndef NO_CACHE_FRONTIER
#define CacheFrontier()                         \
        do {                                    \
                /* Frontier = FrontierMem; */    \
                /* UMFrontier = UMFrontierMem; */     \
        } while (0)
#else
#define CacheFrontier()
#endif

#ifndef NO_CACHE_STACK
#define CacheStackTop()                         \
        do {                                    \
                /*StackTop = StackTopMem;*/         \
        } while (0)
#else
#define CacheStackTop() 
#endif

/* ------------------------------------------------- */
/*                       Chunk                       */
/* ------------------------------------------------- */

#if (defined (__sun__) && defined (REGISTER_FRONTIER_STACKTOP))
#define Chunk(n)                                                \
        DeclareChunk(n) {                                       \
                struct cont cont;                               \
                /* register unsigned int frontier asm("g5"); */       \
                /*uintptr_t l_nextFun = nextFun; */                  \
                register unsigned int stackTop asm("g6");
#else
#define Chunk(n)                                \
        DeclareChunk(n) {                       \
                struct cont cont;               \
     /*          Pointer frontier;  */             \
     /*          Pointer umfrontier; */              \
                /*uintptr_t l_nextFun = nextFun; */ \
                Pointer stackTop, umstackTop;
#endif

#define ChunkSwitch(n)                                                  \
                if (DEBUG_CCODEGEN)                                     \
                        fprintf (stderr, "%s:%d: entering chunk %d  l_nextFun = %d\n", \
                                        __FILE__, __LINE__, n, (int)l_nextFun); \
                CacheFrontier();                                        \
                CacheStackTop();                                        \
                while (1) {                                             \
                top:                                                    \
                switch (l_nextFun) {

#define EndChunk                                                        \
                default:                                                \
                        /* interchunk return */                         \
                        cont.nextFun = l_nextFun;                            \
                        cont.nextChunk = (void*)nextChunks[l_nextFun];    \
                        leaveChunk:                                     \
                                FlushFrontier();                        \
                                FlushStackTop();                        \
                                return cont;                            \
                } /* end switch (l_nextFun) */                          \
                } /* end while (1) */                                   \
        } /* end chunk */

/* ------------------------------------------------- */
/*                Calling SML from C                 */
/* ------------------------------------------------- */

#define Thread_returnToC()                                              \
        do {                                                            \
                if (1 || DEBUG_CCODEGEN)                                     \
                        fprintf (stderr, "%s:%d: Thread_returnToC()\n", \
                                        __FILE__, __LINE__);            \
                returnToC[get_pthread_num()] = TRUE;                          \
                return cont;                                            \
        } while (0)

/* ------------------------------------------------- */
/*                      farJump                      */
/* ------------------------------------------------- */

#define FarJump(n, l)                           \
        do {                                    \
                PrepFarJump(cont, n, l);              \
                goto leaveChunk;                \
        } while (0)

/* ------------------------------------------------- */
/*                       Stack                       */
/* ------------------------------------------------- */


#define UM_CHUNK_PAYLOAD_SIZE 154
#define UM_CHUNK_PAYLOAD_SAFE_REGION 16

typedef uintptr_t GC_returnAddress;

typedef struct GC_UM_Chunk {
    unsigned char ml_object[UM_CHUNK_PAYLOAD_SIZE + UM_CHUNK_PAYLOAD_SAFE_REGION];
    Word32_t chunk_header;
    size_t sentinel;
    struct GC_UM_Chunk* next_chunk;
    struct GC_UM_Chunk* prev_chunk;
    GC_returnAddress ra;
};

#ifdef UMSTACKS
#define Push(bytes)                                                     \
        do {                                                            \
                if (1 || DEBUG_CCODEGEN)                                \
                        fprintf (stderr, "%s:%d: Push (%d)\n",          \
                                        __FILE__, __LINE__, bytes);     \
                if (bytes < 0) {                                        \
                     fprintf(stderr, "%d] umstack: retreat\n", PTHREAD_NUM); \
                     struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                     fprintf(stderr, "   base %016lx prev %016lx\n", cf, cf->prev_chunk); \
                     if (cf->prev_chunk) { \
                         CurrentFrame = cf->prev_chunk; \
                     } else {                                                \
                         fprintf(stderr, "!!!cant retreat to prev frame\n");      \
                     }                                                       \
                } else if (bytes > 0) { \
                     fprintf(stderr, "%d] umstack: advance\n", PTHREAD_NUM); \
                     struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                     fprintf(stderr, "   base %016lx next %016lx\n", cf, cf->next_chunk); \
                     if (cf->next_chunk) { \
                         CurrentFrame = cf->next_chunk; \
                     } else {                                                \
                         fprintf(stderr, "!!!cant advance to next frame\n");      \
                     } \
                } else { fprintf(stderr, "???Push(0)\n"); } \
        } while (0)

#define Return()                                                                \
        do {                                                                    \
                struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                l_nextFun = *(uintptr_t*)(cf - sizeof(void*));            \
                if (1 || DEBUG_CCODEGEN)                                             \
                        fprintf (stderr, "%s:%d: Return()  l_nextFun = %d currentFrame base %016lx\n",   \
                                        __FILE__, __LINE__, (int)l_nextFun,           \
                                        cf);    \
                goto top;                                                       \
        } while (0)

#define Raise()                                                                 \
        do {                                                                    \
                        fprintf (stderr, "%s:%d: Raise *** TODO ***\n",                      \
                                        __FILE__, __LINE__);                    \
                                                     \
        } while (0)

#else

#define Push(bytes)                                                     \
        do {                                                            \
                if (1 || DEBUG_CCODEGEN)                                \
                        fprintf (stderr, "%s:%d: Push (%d)\n",          \
                                        __FILE__, __LINE__, bytes);     \
                StackTop += (bytes);                                    \
        } while (0)

#define Return()                                                                \
        do {                                                                    \
                l_nextFun = *(uintptr_t*)(StackTop - sizeof(void*));            \
                if (DEBUG_CCODEGEN)                                             \
                        fprintf (stderr, "%s:%d: Return()  l_nextFun = %d (%d)\n",   \
                                        __FILE__, __LINE__, (int)l_nextFun, *(uintptr_t*)(UMStackTop - sizeof(void*)));    \
                goto top;                                                       \
        } while (0)

#define Raise()                                                                 \
        do {                                                                    \
                if (DEBUG_CCODEGEN)                                             \
                        fprintf (stderr, "%s:%d: Raise\n",                      \
                                        __FILE__, __LINE__);                    \
                StackTop = StackBottom + ExnStack;                              \
                UMStackTop = UMStackBottom + ExnStack;                          \
                Return();                                                       \
        } while (0)                                                             \

#endif


/* ------------------------------------------------- */
/*                       Primitives                  */
/* ------------------------------------------------- */

#ifndef MLTON_CODEGEN_STATIC_INLINE
#define MLTON_CODEGEN_STATIC_INLINE static inline
#endif
/* Declare inlined math functions, since <math.h> isn't included.
 */
#ifndef MLTON_CODEGEN_MATHFN
#define MLTON_CODEGEN_MATHFN(decl) decl
#endif
/* WordS<N>_quot and WordS<N>_rem can't be inlined with the C-codegen,
 * because the gcc optimizer sometimes produces incorrect results when
 * one of the arguments is a constant.
 */
#ifndef MLTON_CODEGEN_WORDSQUOTREM
#define MLTON_CODEGEN_WORDSQUOTREM(func) PRIVATE
#endif
#ifndef MLTON_CODEGEN_WORDSQUOTREM_IMPL
#define MLTON_CODEGEN_WORDSQUOTREM_IMPL(func)
#endif
/* Declare memcpy, since <string.h> isn't included.
 */
#ifndef MLTON_CODEGEN_MEMCPY
#define MLTON_CODEGEN_MEMCPY(decl)
#endif
MLTON_CODEGEN_MEMCPY(void * memcpy(void *, const void*, size_t);)
#include "basis-ffi.h"
#include "basis/coerce.h"
#include "basis/cpointer.h"
#include "basis/Real/Real-ops.h"
#include "basis/Real/Math-fns.h"
#include "basis/Word/Word-ops.h"
#include "basis/Word/Word-consts.h"
#include "basis/Word/Word-check.h"

/* ------------------------------------------------- */
/*                        Word                       */
/* ------------------------------------------------- */

#define WordS_addCheckCX(size, dst, cW, xW, l)                  \
  do {                                                          \
    WordS##size c = cW;                                         \
    WordS##size x = xW;                                         \
    WordS_addCheckBodyCX(size, c, x, goto l, dst = c + x);      \
  } while (0)
#define WordS8_addCheckCX(dst, c, x, l) WordS_addCheckCX(8, dst, c, x, l)
#define WordS16_addCheckCX(dst, c, x, l) WordS_addCheckCX(16, dst, c, x, l)
#define WordS32_addCheckCX(dst, c, x, l) WordS_addCheckCX(32, dst, c, x, l)
#define WordS64_addCheckCX(dst, c, x, l) WordS_addCheckCX(64, dst, c, x, l)

#define WordS8_addCheckXC(dst, x, c, l) WordS8_addCheckCX(dst, c, x, l)
#define WordS16_addCheckXC(dst, x, c, l) WordS16_addCheckCX(dst, c, x, l)
#define WordS32_addCheckXC(dst, x, c, l) WordS32_addCheckCX(dst, c, x, l)
#define WordS64_addCheckXC(dst, x, c, l) WordS64_addCheckCX(dst, c, x, l)

#define WordS8_addCheck WordS8_addCheckXC
#define WordS16_addCheck WordS16_addCheckXC
#define WordS32_addCheck WordS32_addCheckXC
#define WordS64_addCheck WordS64_addCheckXC


#define WordU_addCheckCX(size, dst, cW, xW, l)                  \
  do {                                                          \
    WordU##size c = cW;                                         \
    WordU##size x = xW;                                         \
    WordU_addCheckBodyCX(size, c, x, goto l, dst = c + x);      \
  } while (0)
#define WordU8_addCheckCX(dst, c, x, l) WordU_addCheckCX(8, dst, c, x, l)
#define WordU16_addCheckCX(dst, c, x, l) WordU_addCheckCX(16, dst, c, x, l)
#define WordU32_addCheckCX(dst, c, x, l) WordU_addCheckCX(32, dst, c, x, l)
#define WordU64_addCheckCX(dst, c, x, l) WordU_addCheckCX(64, dst, c, x, l)

#define WordU8_addCheckXC(dst, x, c, l) WordU8_addCheckCX(dst, c, x, l)
#define WordU16_addCheckXC(dst, x, c, l) WordU16_addCheckCX(dst, c, x, l)
#define WordU32_addCheckXC(dst, x, c, l) WordU32_addCheckCX(dst, c, x, l)
#define WordU64_addCheckXC(dst, x, c, l) WordU64_addCheckCX(dst, c, x, l)

#define WordU8_addCheck WordU8_addCheckXC
#define WordU16_addCheck WordU16_addCheckXC
#define WordU32_addCheck WordU32_addCheckXC
#define WordU64_addCheck WordU64_addCheckXC


#define WordS_negCheck(size, dst, xW, l)                \
  do {                                                  \
    WordS##size x = xW;                                 \
    WordS_negCheckBody(size, x, goto l, dst = -x);      \
  } while (0)
#define Word8_negCheck(dst, x, l) WordS_negCheck(8, dst, x, l)
#define Word16_negCheck(dst, x, l) WordS_negCheck(16, dst, x, l)
#define Word32_negCheck(dst, x, l) WordS_negCheck(32, dst, x, l)
#define Word64_negCheck(dst, x, l) WordS_negCheck(64, dst, x, l)


#define WordS_subCheckCX(size, dst, cW, xW, l)                  \
  do {                                                          \
    WordS##size c = cW;                                         \
    WordS##size x = xW;                                         \
    WordS_subCheckBodyCX(size, c, x, goto l, dst = c - x);      \
  } while (0)
#define WordS8_subCheckCX(dst, c, x, l) WordS_subCheckCX(8, dst, c, x, l)
#define WordS16_subCheckCX(dst, c, x, l) WordS_subCheckCX(16, dst, c, x, l)
#define WordS32_subCheckCX(dst, c, x, l) WordS_subCheckCX(32, dst, c, x, l)
#define WordS64_subCheckCX(dst, c, x, l) WordS_subCheckCX(64, dst, c, x, l)

#define WordS_subCheckXC(size, dst, xW, cW, l)                  \
  do {                                                          \
    WordS##size x = xW;                                         \
    WordS##size c = cW;                                         \
    WordS_subCheckBodyXC(size, x, c, goto l, dst = x - c);      \
  } while (0)
#define WordS8_subCheckXC(dst, x, c, l) WordS_subCheckXC(8, dst, x, c, l)
#define WordS16_subCheckXC(dst, x, c, l) WordS_subCheckXC(16, dst, x, c, l)
#define WordS32_subCheckXC(dst, x, c, l) WordS_subCheckXC(32, dst, x, c, l)
#define WordS64_subCheckXC(dst, x, c, l) WordS_subCheckXC(64, dst, x, c, l)

#define WordS8_subCheck WordS8_subCheckXC
#define WordS16_subCheck WordS16_subCheckXC
#define WordS32_subCheck WordS32_subCheckXC
#define WordS64_subCheck WordS64_subCheckXC


#define WordS_mulCheck(size, dst, xW, yW, l)                    \
  do {                                                          \
    WordS##size x = xW;                                         \
    WordS##size y = yW;                                         \
    WordS_mulCheckBody(size, x, y, goto l, dst = x * y);        \
  } while (0)
#define WordS8_mulCheck(dst, x, y, l) WordS_mulCheck(8, dst, x, y, l)
#define WordS16_mulCheck(dst, x, y, l) WordS_mulCheck(16, dst, x, y, l)
#define WordS32_mulCheck(dst, x, y, l) WordS_mulCheck(32, dst, x, y, l)
#define WordS64_mulCheck(dst, x, y, l) WordS_mulCheck(64, dst, x, y, l)

#define WordU_mulCheck(size, dst, xW, yW, l)                    \
  do {                                                          \
    WordU##size x = xW;                                         \
    WordU##size y = yW;                                         \
    WordU_mulCheckBody(size, x, y, goto l, dst = x * y);        \
  } while (0)
#define WordU8_mulCheck(dst, x, y, l) WordU_mulCheck(8, dst, x, y, l)
#define WordU16_mulCheck(dst, x, y, l) WordU_mulCheck(16, dst, x, y, l)
#define WordU32_mulCheck(dst, x, y, l) WordU_mulCheck(32, dst, x, y, l)
#define WordU64_mulCheck(dst, x, y, l) WordU_mulCheck(64, dst, x, y, l)

#endif /* #ifndef _C_CHUNK_H_ */

