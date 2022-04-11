/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef _C_CHUNK_H_
#define _C_CHUNK_H_

#define STACKLET_DEBUG 0

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <assert.h>

#ifndef PTHREAD_NUM
# define PTHREAD_NUM pthread_num
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

void um_dumpStack (void *s);
void um_dumpFrame (void *s, void *f);


#define WORDWIDTH sizeof(void *)
#define STACKHEADER WORDWIDTH

#define NO_CACHE_STACK
#define NO_CACHE_FRONTIER

#define GCState ((Pointer)&gcState)
#define ExnStack *(Pointer*)(GCState + ExnStackOffset+(PTHREAD_NUM*WORDWIDTH) )
#define CurrentThread *(size_t*)(GCState + CurrentThreadOffset+(PTHREAD_NUM*WORDWIDTH) )
#define FrontierMem *(Pointer*)(GCState + FrontierOffset)
#define UMFrontierMem *(Pointer*)(GCState + UMFrontierOffset)
#define Frontier *(Pointer*)(GCState + FrontierOffset)
#define UMFrontier *(Pointer*)(GCState + UMFrontierOffset)

#define StackBottom (*(Pointer*)(GCState + StackBottomOffset+(PTHREAD_NUM*WORDWIDTH)))
#define StackTopMem (*(Pointer*)(GCState + StackTopOffset+(PTHREAD_NUM*WORDWIDTH)))
#define StackDepth (*(size_t*)(GCState + StackDepthOffset+(PTHREAD_NUM*WORDWIDTH)))
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

#define X(ty, gc_stat, b, i, s, o) X_NORMAL(ty, gc_stat, b, i, s, o)

#define O(ty, b, o) O_NORMAL(ty, b, o)

#define O_NORMAL(ty, b, o) (*(ty*)((b) + (o)))
#define O_DBG(ty, b, o) (*((fprintf(stderr,"%s:%d %d] O : Addr=%018p Val=%018p\n" \
                        ,__FILE__,__LINE__,PTHREAD_NUM, (void*)((b)+(o)),*(ty*)((b) + (o)))), \
                        (ty*)((b) + (o)) ))
                        

#define X_NORMAL(ty, gc_stat, b, i, s, o) (*(ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o))))
#define X_DBG(ty, gc_stat, b, i, s, o) (*((fprintf (stderr, "%s:%d X: Addr=%018p Val=%018p\n", __FILE__, __LINE__, \
                                                 (void*)((b) + ((i) * (s)) + (o)), \
                                                 *(ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o))) \
                                                 )), \
                                                 (ty*)(UM_Array_offset((gc_stat), (b), (i), (s), (o)))))

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


#define CurrentFrame (*(Pointer*)(GCState + CurrentFrameOffset+(PTHREAD_NUM*WORDWIDTH)))
#define NextFrame ((Pointer)((struct GC_UM_Chunk*)(CurrentFrame - STACKHEADER))->next_chunk+STACKHEADER)

#define MLTON_S_NO(ty, i) *(ty*)(StackTop + (i))
#define MLTON_S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: StackTop=%018p Addr=%018p i=%d Val=%018p\n", __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*) StackTop, \
                               (void*)(StackTop + (i)), i, \
                               *(ty*)(StackTop + (i)))) , \
                        (ty*)(StackTop + (i))))




#define NS(ty, i) NS_NORMAL(ty, i)
#define NS_NORMAL(ty, i) *(ty*)(NextFrame + (i))
#define NS_DBG(ty, i)(*((fprintf (stderr, "%s:%d %d] NS: CurrentFrame=%018p NextFrame=%018p \n", \
                                 __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*)CurrentFrame, (void*)NextFrame, i)),\
                                (ty*)(NextFrame + (i))))
#define STACKLET_S(ty, i) *(ty*)(CurrentFrame + (i))
#define STACKLET_DBG_S(ty, i) (*((fprintf (stderr, "%s:%d %d] S: CurrentFrame=%018p Frame+Offset(%d)=%018p CurrentVal=%018p\n", \
                                 __FILE__, __LINE__, PTHREAD_NUM, \
                                (void*)CurrentFrame, i,           \
                                (void*)(CurrentFrame + (i)),      \
                               *(ty*)(CurrentFrame + (i)))),      \
                                (ty*)(CurrentFrame + (i))))

#define LOCK_DEBUG(LN) if(getenv("DEBUG_LOCKS")) {fprintf(stderr, "%d] LOCKDBG %s %s\n", PTHREAD_NUM, __FUNCTION__, LN);}
#define IFED(X) do { int x = X; if (x) { perror("(in codegen code) perror " #X); printf(" rv=%d\n", x); exit(-1); } } while(0)
#define Lock_fl(s) LOCK_DEBUG("Lock_fl"); IFED(pthread_mutex_lock(&s))
#define Unlock_fl(s) LOCK_DEBUG("Unlock_fl"); IFED(pthread_mutex_unlock(&s))

#define ChunkExnHandler ((struct GC_UM_Chunk*)(CurrentFrame - STACKHEADER))->handler
#define ChunkExnLink ((struct GC_UM_Chunk*)(CurrentFrame - STACKHEADER))->link


#if STACKLET_DEBUG > 1
# define S(ty, i) STACKLET_DBG_S(ty, i)
#else
# define S(ty, i) STACKLET_S(ty, i)
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
                uint32_t pthread_num = get_pthread_num();       \
                register unsigned int stackTop asm("g6");
#else
#define Chunk(n)                                \
        DeclareChunk(n) {                       \
                struct cont cont;               \
     /*          Pointer frontier;  */             \
     /*          Pointer umfrontier; */              \
                /*uintptr_t l_nextFun = nextFun; */ \
                uint32_t pthread_num = get_pthread_num();       \
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
 * .. need to find a better way to handle this. if you change GC_UM_Chunk
 * you must keep umheap.h and this definition in sync.
 */

#define UM_CHUNK_PAYLOAD_SIZE 302
#define UM_CHUNK_PAYLOAD_SAFE_REGION 16

typedef uintptr_t GC_returnAddress;
typedef uintptr_t pointer;

// must match umheap.h

typedef struct GC_UM_Chunk {
	unsigned char ml_object[UM_CHUNK_PAYLOAD_SIZE + UM_CHUNK_PAYLOAD_SAFE_REGION];
	Word32_t chunk_header;
	size_t sentinel;
	GC_returnAddress ra;
	GC_returnAddress handler;
	GC_returnAddress link;
        size_t used;
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

  memcpy(curchunk+N, nextchunk+wordwidth, UM_CHUNK_PAYLOAD_SIZE-N-STACKHEADER)
*/


#define RED(x) "\033[1;31m"x"\033[0m"
#define YELLOW(x) "\033[1;33m"x"\033[0m"
#define GREEN(x) "\033[1;32m"x"\033[0m"
#define FW "08"
#define UM_CHUNK_SENTINEL                0x9999
#define UM_ARRAY_SENTINEL                0x9998
#define UM_STACK_SENTINEL                0x9997

#define STACKLET_Push(bytes)                                                     \
        do {                                                            \
                struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)(CurrentFrame - STACKHEADER); \
                assert (cf->sentinel == UM_STACK_SENTINEL); \
                if (bytes < 0) {                                        \
                     struct GC_UM_Chunk *xx = cf; \
                     StackDepth = StackDepth - 1; \
                     if (STACKLET_DEBUG) { \
                        int fnum = *(GC_returnAddress*)(cf->prev_chunk->ml_object + cf->prev_chunk->ra ); \
                        fprintf(stderr, "%s:%d: %d] "GREEN("SKLT_Push")" (%4d) (thr:%x) " \
                                YELLOW("ra:%d")" depth:%d\tbase %"FW"lx cur %"FW"lx prev %"FW"lx ", \
                                __FILE__, __LINE__, PTHREAD_NUM, bytes, CurrentThread, \
                                fnum, StackDepth, xx, \
                                cf, cf->prev_chunk); \
                     } \
                     if (cf->prev_chunk) { \
                         CurrentFrame = (pointer)(cf->prev_chunk) + STACKHEADER; \
                     } else {                                                \
                         if (STACKLET_DEBUG) fprintf(stderr, RED("!!!cant retreat to prev frame"));      \
                     }    if (STACKLET_DEBUG) fprintf(stderr, "\n");                                  \
                } else if (bytes > 0) {   \
                     struct GC_UM_Chunk *xx = cf; \
                     cf->ra = bytes; \
                     /* if only 2 free chunks available on the stack, we must force grow it */ \
                     if (cf->next_chunk->next_chunk == NULL) maybe_growstack(GCState, CurrentThread, TRUE); \
                     int fnum = *(GC_returnAddress*)((void *)&(cf->ml_object) + cf->ra ); \
                     StackDepth = StackDepth + 1; \
                     if (STACKLET_DEBUG)  { \
                        fprintf(stderr, "%s:%d: %d] "GREEN("SKLT_Push")" (%4d) (thr:%x) "\
                             YELLOW("ra:%d")" depth:%d\tbase %"FW"lx cur %"FW"lx next %"FW"lx\n", \
                             __FILE__, __LINE__, PTHREAD_NUM, bytes, CurrentThread, fnum, StackDepth, xx, \
                             cf, cf->next_chunk); \
                     } \
                     if (cf->next_chunk) { \
                         if (UM_CHUNK_PAYLOAD_SIZE-bytes-STACKHEADER < STACKHEADER) \
                             die("impossible no room in next chunk"); \
                         CurrentFrame = (pointer)cf->next_chunk + STACKHEADER; \
                     } else {                                                \
                         die("out of stack"); \
                     } if (STACKLET_DEBUG) fprintf(stderr, "\n"); \
                } else { if (STACKLET_DEBUG) fprintf(stderr, RED("???SKLT_Push(0)\n")); } \
        } while (0)

#define STACKLET_Return()                                                                                           \
        do {                                                                                                        \
                struct GC_UM_Chunk *cf = (struct GC_UM_Chunk *)(CurrentFrame - STACKHEADER);                        \
                assert (cf->sentinel == UM_STACK_SENTINEL || 1!=1);                                                 \
                if (cf->prev_chunk == 0) fprintf(stderr, RED("Cant RETURN from first stack frame\n"));              \
                else if (cf->prev_chunk->ra == 0) fprintf(stderr, RED("RA zero??\n"));                              \
                else {                                                                                              \
                    l_nextFun = *(GC_returnAddress*)(cf->prev_chunk->ml_object + cf->prev_chunk->ra);               \
                    if (STACKLET_DEBUG || DEBUG_CCODEGEN)                                                           \
                            fprintf (stderr, GREEN("%s:%d: "GREEN("SKLT_Return()")                                  \
                                            "  %d/%x l_nextFun = %d currentFrame %"FW"lx prev %"FW"lx ra %d\n"),    \
                                            __FILE__, __LINE__, PTHREAD_NUM, CurrentThread, (int)l_nextFun,         \
                                            cf, cf->prev_chunk, cf->prev_chunk->ra);                                \
                    goto top;                                                                                       \
                }                                                                                                   \
        } while (0)

#define STACKLET_Raise()                                                                \
        do {                                                                    \
                struct GC_UM_Chunk *cf = (ExnStack - STACKHEADER);  \
                assert (cf->sentinel == UM_STACK_SENTINEL || 2!=2); \
                if (STACKLET_DEBUG) fprintf (stderr, RED("%s:%d: SKLT_Raise exn %x cur %x prev %x\n"),   \
                         __FILE__, __LINE__, ExnStack, cf, cf->prev_chunk);                       \
                if (cf->prev_chunk == 0) fprintf(stderr, RED("Cant RAISE if null prev_chunk\n")); \
                else if (cf->handler == 0) fprintf(stderr, RED("Raise handler zero??\n")); \
                else { \
                    l_nextFun = cf->handler; \
                    /* see discussion in backend.fun's SetExnStackLocal for explanation of next line */ \
                    CurrentFrame = STACKHEADER+(Pointer)(((struct GC_UM_Chunk *)(ExnStack-STACKHEADER))->next_chunk); \
                    if (STACKLET_DEBUG || DEBUG_CCODEGEN)                                             \
                            fprintf (stderr, GREEN("%s:%d: "GREEN("SKLT_RaiseReturn()")\
                                            "  l_nextFun = %d currentFrame %"FW"lx prev %"FW"lx\n"),   \
                                            __FILE__, __LINE__, (int)l_nextFun,           \
                                            cf, cf->prev_chunk);    \
                    goto top;                                                       \
                } \
        } while (0)

/* leaving this in for ref for now */
#if 0
#define MLTON_Push(bytes)                                                     \
        do {                                                                  \
                if (1 || DEBUG_CCODEGEN)    {                                 \
                        int used = StackTop - StackBottom; \
                        fprintf (stderr, "%s:%d: MLTON_Push (%d) %d %"FW"lx %"FW"lx %"FW"lx\n",          \
                                        __FILE__, __LINE__, bytes, used, StackBottom, \
                                        StackTop, StackTop+bytes );     \
                } if(bytes > 0 && STACKLET_DEBUG > 1) {dump_hex(StackTop, bytes); fprintf(stderr, "\n");}StackTop += (bytes);                                    \
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
#endif

#define Push STACKLET_Push
#define Return STACKLET_Return
#define Raise STACKLET_Raise


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

