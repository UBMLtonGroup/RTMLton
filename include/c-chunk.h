/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _C_CHUNK_H_
#define _C_CHUNK_H_

#define STACKLETS
#define STACKLET_DEBUG FALSE

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>

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

#define WORDWIDTH 4 /* use gcState->alignment */

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

#define RTSync *(bool*)(GCState + RTSyncOffset + (PTHREAD_NUM *WORDWIDTH)) 

#define StackTop StackTopMem

#define CASLOCK CompareAndSet(GCState,1)
#define CASUNLOCK CompareAndSet(GCState,0)

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
#if 1

#define O(ty, b, o) (*(ty*)((b) + (o)))
// #define X(ty, b, i, s, o) (*(ty*)((b) + ((i) * (s)) + (o)))
#define X(ty, gc_stat, b, i, s, o) (*(ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o))))
//#define S(ty, i) *(ty*)(StackTop + (i))
#define CHOFF(gc_stat, ty, b, o, s) (*(ty*)(UM_Chunk_Next_offset((gc_stat), (b), (o), (s))))
//#define WB(ty,d,s,db,sb)  writeBarrier(GCState,(db),(sb)); d=s
#define WB(ty,db,sb,d,s,op)                                         \
            do {                                                    \
                    if(!RTSync)                                     \
                        d=s;                                        \
                    else                                            \
                    {   CASLOCK;                                    \
                        writeBarrier(GCState,(db),(sb));            \
                        d=s;                                        \
                        CASUNLOCK;                                  \
                    }                                               \
                } while(0)                                          \


#define CurrentFrameOffset 52
#define CurrentFrame (*(Pointer*)(GCState + CurrentFrameOffset+(PTHREAD_NUM*WORDWIDTH)))

#define MLTON_S_NO(ty, i) *(ty*)(StackTop + (i))
#define MLTON_S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: StackTop=%018p Addr=%018p i=%d Val=%018p\n", __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*) StackTop, \
                               (void*)(StackTop + (i)), i, \
                               *(ty*)(StackTop + (i)))) , \
                        (ty*)(StackTop + (i))))

#define STACKLET_S(ty, i) *(ty*)(CurrentFrame + (i) + WORDWIDTH)
#define STACKLET_DBG_S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: CurrentFrame=%018p Frame+Offset(%d)=%018p CurrentVal=%018p\n", \
                                 __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*)CurrentFrame, i, \
                                (void*)(CurrentFrame + (i) + WORDWIDTH), \
                               *(ty*)(CurrentFrame + (i)  + WORDWIDTH))) , \
                                (ty*)(CurrentFrame + (i)  + WORDWIDTH)))

#if STACKLET_DEBUG == TRUE
# define STACKWRITE(A, V) fprintf(stderr, "%s:%d %d] WRITE: cur:%x new:%x\n", __FILE__, __LINE__, PTHREAD_NUM, A, V)
#else
# define STACKWRITE(A, V)
#endif

#define ChunkExnHandler ((struct GC_UM_Chunk*)CurrentFrame)->handler
#define ChunkExnLink ((struct GC_UM_Chunk*)CurrentFrame)->link


#ifdef STACKLETS
# if STACKLET_DEBUG == TRUE
#  define S(ty, i) STACKLET_DBG_S(ty, i)
# else
#  define S(ty, i) STACKLET_S(ty, i)
# endif
#else
# define S(ty, i) MLTON_S(ty, i)
#endif

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
                Pointer stackTop;
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
                if (DEBUG_CCODEGEN)                                     \
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


/* this comes from umheap.h which isnt available outside of the runtime
 * .. need to find a better way to handle this
 */

#define UM_CHUNK_PAYLOAD_SIZE 154
#define UM_CHUNK_PAYLOAD_SAFE_REGION 16
#define CURRENTTHREAD_OFFSET 588

typedef uintptr_t GC_returnAddress;
typedef uintptr_t pointer;

typedef struct GC_UM_Chunk {
	unsigned char ml_object[UM_CHUNK_PAYLOAD_SIZE + UM_CHUNK_PAYLOAD_SAFE_REGION];
	Word32_t chunk_header;
	size_t sentinel;
	GC_returnAddress ra;
	GC_returnAddress handler;
	GC_returnAddress link;
	void *memcpy_addr;
	size_t memcpy_size;
	struct GC_UM_Chunk* next_chunk;
	struct GC_UM_Chunk* prev_chunk;
};

void dump_hex(char *str, int len);

/* end of umheap.h copy/paste */

/*
  when stacklet_push(N) is called, we likely have already written
  into the next frame (eg arguments and/or returnvalue object pointers)
  but in the chunked model those have been written into the current
  chunk. we need to move those to the next chunk so that they are in
  the correct stack frame. we will XXX FIX inefficiently memcpy as follows:

  memcpy(curchunk+N, nextchunk+wordwidth, UM_CHUNK_PAYLOAD_SIZE-N-WORDWIDTH)
*/


#define RED(x) "\033[1;31m"x"\033[0m"
#define YELLOW(x) "\033[1;33m"x"\033[0m"
#define GREEN(x) "\033[1;32m"x"\033[0m"
#define FW "08"

#define STACKLET_Push(bytes)                                                     \
        do {                                                            \
                if (bytes < 0) {                                        \
                     struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                     struct GC_UM_Chunk *xx = cf; \
                     if (STACKLET_DEBUG) { \
                        for ( ; xx && xx->prev_chunk ; xx = xx->prev_chunk); /* find the 1st chunk just so we can print the addr */ \
                        fprintf(stderr, "%s:%d: SKLT_Push (%4d)\tbase %"FW"lx cur %"FW"lx prev %"FW"lx " \
                                                "SB[%"FW"lx] ST[%"FW"lx] ", \
                                __FILE__, __LINE__, bytes, xx, \
                                cf, cf->prev_chunk, StackBottom, StackTop); \
                     } \
                     if (cf->prev_chunk) { \
                         CurrentFrame = cf->prev_chunk; \
                         if (STACKLET_DEBUG) fprintf(stderr, "   ra=%d ", cf->prev_chunk->ra); \
                     } else {                                                \
                         if (STACKLET_DEBUG) fprintf(stderr, RED("!!!cant retreat to prev frame"));      \
                     }    if (STACKLET_DEBUG) fprintf(stderr, "\n");                                  \
                } else if (bytes > 0) { \
                     struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                     struct GC_UM_Chunk *xx = cf; \
                     cf->ra = bytes; \
                     if (STACKLET_DEBUG)  { \
                        for ( ; xx && xx->prev_chunk ; xx = xx->prev_chunk); /* find the 1st chunk just so we can print the addr */ \
                        fprintf(stderr, "%s:%d: SKLT_Push (%4d)\tbase %"FW"lx cur %"FW"lx next %"FW"lx SB[%"FW"lx] ST[%"FW"lx]\n", \
                             __FILE__, __LINE__, bytes, xx, \
                             cf, cf->next_chunk, StackBottom, StackTop); \
                        fprintf(stderr, YELLOW("current chunk:\n")); \
                        dump_hex(cf, bytes+WORDWIDTH+32);\
                     } \
                     if (cf->next_chunk) { \
                         if (UM_CHUNK_PAYLOAD_SIZE-bytes-WORDWIDTH < WORDWIDTH) die("impossible no room in next chunk"); \
                         if (STACKLET_DEBUG) fprintf(stderr, RED("memcpy: ") "src %"FW"lx dst %"FW"lx\n", cf->ml_object+bytes+WORDWIDTH, cf->next_chunk->ml_object+WORDWIDTH ); \
                         if (STACKLET_DEBUG) fprintf(stderr, "cf %"FW"lx cf->next %"FW"lx bytes %d len %d\n", cf, cf->next_chunk, bytes,UM_CHUNK_PAYLOAD_SIZE-bytes-WORDWIDTH ); \
                         memcpy(cf->next_chunk->ml_object+WORDWIDTH, cf->ml_object+bytes+WORDWIDTH, UM_CHUNK_PAYLOAD_SIZE-bytes-WORDWIDTH); \
                         cf->next_chunk->memcpy_addr = cf->ml_object+bytes+WORDWIDTH; \
                         cf->next_chunk->memcpy_size = UM_CHUNK_PAYLOAD_SIZE-bytes-WORDWIDTH; \
                         CurrentFrame = cf->next_chunk; \
                         if(STACKLET_DEBUG) fprintf(stderr, YELLOW("\nnext_chunk:\n"));\
                         if(STACKLET_DEBUG) dump_hex(cf->next_chunk, 100);\
                     } else {                                                \
                         if (STACKLET_DEBUG) fprintf(stderr, RED("!!!cant advance to next frame"));      \
                     } if (STACKLET_DEBUG) fprintf(stderr, "\n"); \
                } else { if (STACKLET_DEBUG) fprintf(stderr, RED("???SKLT_Push(0)\n")); } \
        } while (0)

#define STACKLET_Return()                                                                \
        do {                                                                    \
                struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)CurrentFrame; \
                if (cf->prev_chunk == 0) fprintf(stderr, RED("Cant RETURN from first stack frame\n")); \
                else if (cf->prev_chunk->ra == 0) fprintf(stderr, RED("RA zero??\n")); \
                else { \
                    memcpy(cf->memcpy_addr, cf->ml_object+WORDWIDTH, cf->memcpy_size); \
                    l_nextFun = (cf->prev_chunk->ml_object[cf->prev_chunk->ra]); \
                    if (STACKLET_DEBUG || DEBUG_CCODEGEN)                                             \
                            fprintf (stderr, GREEN("%s:%d: "GREEN("SKLT_Return()")"  l_nextFun = %d currentFrame %"FW"lx prev %"FW"lx ra %d\n"),   \
                                            __FILE__, __LINE__, (int)l_nextFun,           \
                                            cf, cf->prev_chunk, cf->prev_chunk->ra);    \
                    goto top;                                                       \
                } \
        } while (0)

#define STACKLET_Raise()                                                                \
        do {                                                                    \
                struct GC_UM_Chunk *cf = ExnStack;  \
                if (STACKLET_DEBUG) fprintf (stderr, RED("%s:%d: SKLT_Raise exn %x cur %x prev %x\n"),   \
                         __FILE__, __LINE__, ExnStack, cf, cf->prev_chunk);                       \
                if (cf->prev_chunk == 0) fprintf(stderr, RED("Cant RAISE if null prev_chunk\n")); \
                else if (cf->handler == 0) fprintf(stderr, RED("Raise handler zero??\n")); \
                else { \
                    l_nextFun = cf->handler; CurrentFrame = ((struct GC_UM_Chunk *)ExnStack)->next_chunk; \
                    if (STACKLET_DEBUG || DEBUG_CCODEGEN)                                             \
                            fprintf (stderr, GREEN("%s:%d: "GREEN("SKLT_RaiseReturn()")"  l_nextFun = %d currentFrame %"FW"lx prev %"FW"lx\n"),   \
                                            __FILE__, __LINE__, (int)l_nextFun,           \
                                            cf, cf->prev_chunk);    \
                    goto top;                                                       \
                } \
        } while (0)

#define MLTON_Push(bytes)                                                     \
        do {                                                                  \
                if (1 || DEBUG_CCODEGEN)    {                                 \
                        int used = StackTop - StackBottom; \
                        fprintf (stderr, "%s:%d: MLTON_Push (%d) %d %"FW"lx %"FW"lx %"FW"lx\n",          \
                                        __FILE__, __LINE__, bytes, used, StackBottom, \
                                        StackTop, StackTop+bytes );     \
                } if(bytes > 0) {dump_hex(StackTop, bytes); fprintf(stderr, "\n");}StackTop += (bytes);                                    \
        } while (0);

#define MLTON_Return()                                                                \
        do {                                                                    \
                l_nextFun = *(uintptr_t*)(StackTop - sizeof(void*));            \
                if (1 || DEBUG_CCODEGEN)                                             \
                        fprintf (stderr, "%s:%d: "GREEN("MLTON_Return()")"  l_nextFun = %d\n",   \
                                        __FILE__, __LINE__, (int)l_nextFun);    \
                goto top;                                                       \
        } while (0)

#define MLTON_Raise()                                                                 \
        do {                                                                    \
                if (1 || DEBUG_CCODEGEN)                                        \
                        fprintf (stderr, "%s:%d: "RED("MLTON_Raise")"\n",             \
                                        __FILE__, __LINE__);                    \
                StackTop = StackBottom + ExnStack;                              \
                Return();                                                       \
        } while (0)

#ifdef STACKLETS
#define Push STACKLET_Push
#define Return STACKLET_Return
#define Raise STACKLET_Raise
#else
#define Push MLTON_Push
#define Return MLTON_Return
#define Raise MLTON_Raise
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

