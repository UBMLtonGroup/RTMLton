/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#ifndef DEBUG
#define DEBUG FALSE
#endif

//#define PROFILE_UMGC

enum {
  DEBUG_ALLOC = FALSE,
  DEBUG_ALLOC_PACK = FALSE,
  DEBUG_ARRAY = FALSE,
  DEBUG_ARRAY_OFFSET = FALSE,
  DEBUG_CALL_STACK = FALSE,
  DEBUG_CARD_MARKING = FALSE,
  DEBUG_CHUNK_ARRAY = FALSE,
  DEBUG_DETAILED = FALSE,
  DEBUG_DFS_MARK = FALSE,
  DEBUG_ENTER_LEAVE = FALSE,
  DEBUG_GENERATIONAL = FALSE,
  DEBUG_INT_INF = FALSE,
  DEBUG_INT_INF_DETAILED = FALSE,
  DEBUG_LOCKS = FALSE,
  DEBUG_MARK_COMPACT = FALSE,
  DEBUG_MEM = FALSE,
  DEBUG_OBJPTR = FALSE,
  DEBUG_PROFILE = FALSE,
  DEBUG_RESIZING = FALSE,
  DEBUG_SHARE = FALSE,
  DEBUG_SIGNALS = FALSE,
  DEBUG_SIZE = FALSE,
  DEBUG_SOURCES = FALSE,
  DEBUG_STACKS = FALSE,
  DEBUG_STACK_GROW = FALSE,
  DEBUG_THREADS = FALSE,
  DEBUG_WEAK = FALSE,
  DEBUG_WORLD = FALSE,
  FORCE_GENERATIONAL = FALSE,
  FORCE_MARK_COMPACT = FALSE,
  DEBUG_OLD = FALSE,
  DEBUG_RTGC = FALSE,
  DEBUG_RTGC_MARKING = FALSE,
  DEBUG_RTGC_MARKING_SHADING = FALSE,
  DEBUG_RTGC_VERBOSE = FALSE,
  DEBUG_WB = FALSE,
  DISPLAY_GC_STATS = FALSE
};

#define RED(x) "\033[1;31m"x"\033[0m"
#define YELLOW(x) "\033[1;33m"x"\033[0m"
#define GREEN(x) "\033[1;32m"x"\033[0m"
#define BLUE(x) "\033[1;34m"x"\033[0m"
#define PURPLE(x) "\033[1;35m"x"\033[0m"

#ifdef DO_PERF
# define START_PERF 	struct timeval t0, t1; gettimeofday(&t0, NULL)
# define STOP_PERF do { gettimeofday(&t1, NULL); unsigned int xxx = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec); fprintf(stderr, "%d] PERF %s %d us\n", PTHREAD_NUM, __FUNCTION__, xxx); } while(0)
#else
# define START_PERF do {} while(0)
# define STOP_PERF do {} while(0)
#endif

#ifdef DO_LOCK_DEBUG
# define LOCK_DEBUG(LN) if(getenv("DEBUG_LOCKS")) {fprintf(stderr, "%d] LOCKDBG %s %s\n", PTHREAD_NUM, __FUNCTION__, LN);}
# define LOCK_DEBUG2(LNAME, LNUM) if(getenv("DEBUG_LOCKS")) {fprintf(stderr, "%d] LOCKDBG %s %s(%d)\n", PTHREAD_NUM, __FUNCTION__, LNAME, LNUM);}
#else
# define LOCK_DEBUG(LN)
# define LOCK_DEBUG2(LNAME, LNUM)
#endif
