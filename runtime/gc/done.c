/* Copyright (C) 2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)
#define LOCK_FL IFED(pthread_mutex_lock(&s->fl_lock))
#define UNLOCK_FL IFED(pthread_mutex_unlock(&s->fl_lock))

#define BLOCK IFED(pthread_cond_wait(&s->fl_empty_cond,&s->fl_lock))

static void displayCol (FILE *out, size_t width, const char *s) {
  size_t extra;
  size_t i;
  size_t len;

  len = strlen (s);
  if (len < width) {
    extra = width - len;
    for (i = 0; i < extra; i++)
      fprintf (out, " ");
  }
  fprintf (out, "%s\t", s);
}

static void displayCollectionStats (FILE *out, const char *name, struct rusage *ru,
                                    uintmax_t num, uintmax_t bytes) {
  uintmax_t ms;

  ms = rusageTime (ru);
  fprintf (out, "%s", name);
  displayCol (out, 7, uintmaxToCommaString (ms));
  displayCol (out, 7, uintmaxToCommaString (num));
  displayCol (out, 15, uintmaxToCommaString (bytes));
  displayCol (out, 15,
              (ms > 0)
              ? uintmaxToCommaString ((uintmax_t)(1000.0 * (float)bytes/(float)ms))
              : "-");
  fprintf (out, "\n");
}

static void displayChunkedGCStats(GC_state s, FILE *out)
{
    fprintf (out, "-------------\t-------\t-------\t---------------\t---------------\n");
    fprintf(out,"GC Statistics\n");
    fprintf (out, "-------------\t-------\t-------\t---------------\t---------------\n");
    fprintf(out,"Number of Chunks allocated = %s\n",uintmaxToCommaString (s->cGCStats.numChunksAllocated));
    fprintf(out,"Number of Chunks Freed = %s\n",uintmaxToCommaString (s->cGCStats.numChunksFreed));
    fprintf(out,"Number of GC Sweeps = %s\n",uintmaxToCommaString (s->cGCStats.numSweeps));
    fprintf(out,"Total GC Time = %s\n",uintmaxToCommaString (s->cGCStats.totalGCTime));
    fprintf(out,"Mutator was blocked %d times\n",s->blocked);
    fprintf(out,"Max mutator pause time = %s\n",uintmaxToCommaString (s->cGCStats.maxMutatorPauseTime));
    fprintf(out,"Allocted by RT thread = %s\n",uintmaxToCommaString (s->allocedByRT));


}

void RTGC_done(GC_state s)
{
    FILE *out;
    out = stderr;

    /*If GC is running wait till its done*/
    if(s->isGCRunning)
    {
        LOCK_FL;
        if(DEBUG_RTGC)
            fprintf(stderr,"%d] Main thread blocking until GC is done\n",PTHREAD_NUM);

        BLOCK;
        
        if(DEBUG_RTGC)
            fprintf(stderr,"%d] Main thread Unblocked by GC\n",PTHREAD_NUM);
        UNLOCK_FL;
    }

    free(s->worklist);


    displayChunkedGCStats(s,out);
}

void GC_done (GC_state s) {


  if (s->gc_module == GC_NONE ||
      s->gc_module == GC_UM) {
      return;
  }
  FILE *out;

  enter (s);
  minorGC (s);
  out = stderr;
  if (s->controls.summary) {
    struct rusage ru_total;
    uintmax_t gcTime;
    uintmax_t totalTime;

    getrusage (RUSAGE_SELF, &ru_total);
    totalTime = rusageTime (&ru_total);
    gcTime = rusageTime (&s->cumulativeStatistics.ru_gc);
    fprintf (out, "GC type\t\ttime ms\t number\t\t  bytes\t      bytes/sec\n");
    fprintf (out, "-------------\t-------\t-------\t---------------\t---------------\n");
    displayCollectionStats
      (out, "copying\t\t",
       &s->cumulativeStatistics.ru_gcCopying,
       s->cumulativeStatistics.numCopyingGCs,
       s->cumulativeStatistics.bytesCopied);
    displayCollectionStats
      (out, "mark-compact\t",
       &s->cumulativeStatistics.ru_gcMarkCompact,
       s->cumulativeStatistics.numMarkCompactGCs,
       s->cumulativeStatistics.bytesMarkCompacted);
    displayCollectionStats
      (out, "minor\t\t",
       &s->cumulativeStatistics.ru_gcMinor,
       s->cumulativeStatistics.numMinorGCs,
       s->cumulativeStatistics.bytesCopiedMinor);
    fprintf (out, "total time: %s ms\n",
             uintmaxToCommaString (totalTime));
    fprintf (out, "total GC time: %s ms (%.1f%%)\n",
             uintmaxToCommaString (gcTime),
             (0 == totalTime)
             ? 0.0
             : 100.0 * ((double) gcTime) / (double)totalTime);
    fprintf (out, "max pause time: %s ms\n",
             uintmaxToCommaString (s->cumulativeStatistics.maxPauseTime));
    fprintf (out, "total bytes allocated: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.bytesAllocated));
    fprintf (out, "max bytes live: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.maxBytesLive));
    fprintf (out, "max heap size: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.maxHeapSize));
    fprintf (out, "max stack size: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.maxStackSize));
    fprintf (out, "num cards marked: %s\n",
             uintmaxToCommaString (s->cumulativeStatistics.numCardsMarked));
    fprintf (out, "bytes scanned: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.bytesScannedMinor));
    fprintf (out, "bytes hash consed: %s bytes\n",
             uintmaxToCommaString (s->cumulativeStatistics.bytesHashConsed));
  }
  releaseHeap (s, &s->heap);
  releaseHeap (s, &s->secondaryHeap);
}
