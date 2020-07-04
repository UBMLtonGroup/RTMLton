#include "../gc.h"


#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)
#define LOCK_FL IFED(pthread_mutex_lock(&s->fl_lock))
#define UNLOCK_FL IFED(pthread_mutex_unlock(&s->fl_lock))

#define BLOCK IFED(pthread_cond_wait(&s->fl_empty_cond,&s->fl_lock))


void initUMHeap(GC_state s,
				GC_UM_heap h) {
	h->start = NULL;
	h->size = 0;
	h->end = NULL;
	h->limit = NULL;
	h->fl_head = NULL;
	h->fl_tail = NULL;
    h->sl_head = NULL;
    h->sl_tail = NULL;
	s->fl_chunks = 0;
    s->sl_chunks = 0;
	s->maxChunksAvailable = 0;
	s->heuristicChunks = 0;
}

#ifdef STACK_GC_SANITY
extern GC_UM_Chunk stack_list[];
extern unsigned int stack_list_end;
#endif


GC_UM_Chunk allocNextChunk(GC_state s,
						   GC_UM_heap h,
						   UM_header chunkType) {
	/*Only place this should be called is allocChunk to preserve fl_lock*/

	/*Allocate next chunk from start of free list*/
	h->fl_head->chunkType = chunkType;
	struct UM_Mem_Chunk *nc = h->fl_head->next_chunk;
	GC_UM_Chunk c = (GC_UM_Chunk)((pointer) h->fl_head + sizeof(UM_header));
	c->next_chunk = c->prev_chunk = NULL;
	c->sentinel = UM_CHUNK_SENTINEL;
	c->chunk_header = UM_CHUNK_HEADER_CLEAN;

	if (nc == NULL) {
		h->fl_head = NULL;
		h->fl_tail = NULL;
	} else {
		h->fl_head = nc;
	}

	if (s->rtSync[PTHREAD_NUM]) {
		c->chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
	} else {
		c->chunk_header |= UM_CHUNK_RED_MASK;
	}
	s->fl_chunks -= 1;
	s->reserved -= 1;
	s->cGCStats.numChunksAllocated++;

	return c;
}

#if 0
/*Should be called only from allocNextchunk or allocNextArray Chunk because it unlocks fl_lock*/
void blockAllocator(GC_state s,size_t numChunks)
{
	if(DEBUG_RTGC)
		fprintf(stderr,"%d] Going to block for GC, FC =%d\n",PTHREAD_NUM,s->fl_chunks);

	size_t fc_BeforeBlock = s->fl_chunks;

  /*If RTSync has not been set, a.k.a thread has not been marked, mark it*/
	if(!s->rtSync[PTHREAD_NUM])
		GC_collect(s,0,false, false);

 /*If GC is not running before mutator sleeps, keep sending signal till GC wakes up
  * Race occurs when GC is not running and mutator sleeps.*/
	while(!s->isGCRunning)
		{
			RTSYNC_SIGNAL;
			if(DEBUG_RTGC)
				fprintf(stderr,"%d] Signal sent to wake GC before blocking\n",PTHREAD_NUM);
		}


	/*Blocks on cond variable , automatically unlocks s->fl_lock*/
	BLOCK;

	if(DEBUG_RTGC)
		fprintf(stderr,"%d] Back from waiting for GC to clear chunks, FC =%d\n",PTHREAD_NUM,s->fl_chunks);

	if(!(s->fl_chunks > fc_BeforeBlock) || !(s->fl_chunks > numChunks)) {
		fprintf(stderr, "fl_chunks %d beforeblock %d, numchunks %d\n",
				s->fl_chunks, fc_BeforeBlock, numChunks);
		die("allocNextChunk: No more memory available\n");
	}

}
#endif

GC_UM_Chunk allocateChunks(GC_state s, GC_UM_heap h, size_t numChunks, UM_header chunkType) {

	if (numChunks > s->maxChunksAvailable) {
		die("Insufficient Memory: Asking for more memory than total heap space\n");
	}

	LOCK_FL;

	/*if (s->fl_chunks < 1 || s->fl_chunks < numChunks)
		{
		  blockAllocator(s,numChunks);
		}
	*/

	assert(numChunks <= s->reserved);

	GC_UM_Chunk head = allocNextChunk(s, &(s->umheap), chunkType);
	head->chunk_header |= UM_CHUNK_IN_USE;

	if (numChunks > 1) {
		int i;
		GC_UM_Chunk current = head;
		for (i = 0; i < (numChunks - 1); i++) {
			current->next_chunk = allocNextChunk(s, &(s->umheap), chunkType);
			current->next_chunk->chunk_header |= UM_CHUNK_IN_USE;
			current->next_chunk->prev_chunk = current;
			current = current->next_chunk;
		}
	}

	UNLOCK_FL;

	return head;
}


GC_UM_Array_Chunk allocNextArrayChunk(GC_state s,
									  GC_UM_heap h) {


	/*should be called only from allocateArrayChunks to keep sanity of fl_lock*/

	/*Allocate next chunk from start of list*/
	h->fl_head->chunkType = UM_ARRAY_CHUNK;
	struct UM_Mem_Chunk *nc = h->fl_head->next_chunk;
	GC_UM_Array_Chunk c = insertArrayFreeChunk(s, h, ((pointer) h->fl_head +
													  sizeof(UM_header))); /*pass pointer to area after chunktype*/

	if (nc == NULL) {
		h->fl_head = NULL;
		h->fl_tail = NULL;
	} else {
		h->fl_head = nc;
	}

		memset(c, 0xAB, sizeof(struct GC_UM_Array_Chunk)); // jeff - remove
	c->next_chunk = NULL;
	c->array_chunk_magic = UM_ARRAY_SENTINEL;
	c->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
	if (s->rtSync[PTHREAD_NUM]) {
		c->array_chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
	} else {
		c->array_chunk_header |= UM_CHUNK_RED_MASK;
	}
	int i;
	for (i = 0; i < UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
		c->ml_array_payload.um_array_pointers[i] = NULL;
	}
	s->fl_chunks -= 1;
	s->reserved -= 1;
	s->cGCStats.numChunksAllocated++;
	return c;
}

GC_UM_Array_Chunk allocateArrayChunks(GC_state s, GC_UM_heap h, size_t numChunks) {
	assert (numChunks > 0);

	if (numChunks > s->maxChunksAvailable) {
		die("Insufficient Memory: Asking for more memory than total heap space\n");
	}


	LOCK_FL;

	assert(numChunks <= s->reserved);

	GC_UM_Array_Chunk head = allocNextArrayChunk(s, &(s->umheap));

	if (numChunks > 1) {
		int i;
		GC_UM_Array_Chunk current = head;
		for (i = 0; i < (numChunks - 1); i++) {
			current->next_chunk = allocNextArrayChunk(s, &(s->umheap));
			current = current->next_chunk;
		}
	}

	UNLOCK_FL;

	return head;


}


void insertChunkToFL(GC_state s,
					 GC_UM_heap h,
					 pointer c) {
	LOCK_FL;

	/*Insert free chunk to back of free list*/
	UM_Mem_Chunk pc = (UM_Mem_Chunk) c;
	if (s->fl_chunks == 0) {
		h->fl_head = pc;
		h->fl_tail = pc;
		pc->chunkType = UM_EMPTY;
		pc->next_chunk = NULL;
		s->fl_chunks += 1;
	} else {
		h->fl_tail->next_chunk = pc;
		pc->next_chunk = NULL;
		pc->chunkType = UM_EMPTY;
		h->fl_tail = pc;
		s->fl_chunks += 1;
	}


#if 0
	fprintf(stderr, YELLOW("add %8x to free list (clear using 0xAA) %d\n"),
			(unsigned int)pc, UM_CHUNK_PAYLOAD_SIZE);
	memset(pc->ml_object, 0xaa, UM_CHUNK_PAYLOAD_SIZE);
#endif

#ifdef STACK_GC_SANITY
	// TODO jeff - sanity check to make sure stacks are not collected (yet)
	// TODO remove before benchmarking
	if (DEBUG_STACKS) {
		for (unsigned int i = 0; i < stack_list_end; i++) {
			if (stack_list[i] == (GC_UM_Chunk) pc) {
				fprintf(stderr, RED("stack frame added to free list\n"));
				die("abort");
			}
		}
	}
#endif
	UNLOCK_FL;

}

/*No one except GC thread should be calling this*/
void insertChunktoSubList(GC_state s, GC_UM_heap h, pointer c) {

/*Insert free chunk to back of sub list*/
	UM_Mem_Chunk pc = (UM_Mem_Chunk) c;
	if (s->sl_chunks == 0) {
		h->sl_head = pc;
		h->sl_tail = pc;
		pc->chunkType = UM_EMPTY;
		pc->next_chunk = NULL;
        s->sl_chunks += 1;
	} else {
		h->sl_tail->next_chunk = pc;
		pc->next_chunk = NULL;
		pc->chunkType = UM_EMPTY;
		h->sl_tail = pc;
        s->sl_chunks +=1;
	}


}



void addSweepListToFL(GC_state s,GC_UM_heap h){

    if(s->sl_chunks == 0)
        return;
    else
    {
        assert((h->sl_head != NULL) && (h->sl_tail != NULL));
        LOCK_FL;

         
        if (s->fl_chunks == 0) {
            h->fl_head = h->sl_head;
            h->fl_tail = h->sl_tail;
            s->fl_chunks += s->sl_chunks;
        } else {
            h->fl_tail->next_chunk = h->sl_head;
            h->fl_tail = h->sl_tail;
            s->fl_chunks += s->sl_chunks;
        }
        
        /*clear the sub list*/
        h->sl_head = NULL;
        h->sl_tail = NULL;
        s->sl_chunks =0;



        UNLOCK_FL;
    }



}



GC_UM_Array_Chunk insertArrayFreeChunk(GC_state s,
									   GC_UM_heap h,
									   pointer c) {


	GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk) c;
	//    memset(pc->ml_array_payload.ml_object, 0, UM_CHUNK_ARRAY_PAYLOAD_SIZE);
	pc->next_chunk = NULL;
	pc->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
	//h->fl_head = pc;
	//s->fl_chunks += 1;
	return pc;
}


bool createUMHeap(GC_state s,
				  GC_UM_heap h,
				  size_t desiredSize,
				  __attribute__ ((unused)) size_t minSize) {
	pointer newStart;
	newStart = GC_mmapAnon(NULL, desiredSize);;

	if (newStart == (void *) -1) {
		fprintf(stderr, "[GC: MMap Failure]\n");
		return FALSE;
	}

	h->start = newStart;
	h->size = desiredSize;
	h->end = newStart + desiredSize;

	pointer pchunk;
	size_t max_chunk_size = max(sizeof(struct GC_UM_Chunk), sizeof(struct GC_UM_Array_Chunk));
	size_t step = max_chunk_size + sizeof(UM_header); /*account for size of chunktype field*/
	pointer end = h->start + h->size - step;
	if (DEBUG) {
		fprintf(stderr, "%d] sizeof(struct GC_UM_Chunk) = %d\n", PTHREAD_NUM, sizeof(struct GC_UM_Chunk));
		fprintf(stderr, "    sizeof(struct GC_UM_Array_Chunk) = %d\n", sizeof(struct GC_UM_Array_Chunk));
		fprintf(stderr, "    final chunk size = %d\n", max_chunk_size);
	}
	struct timeval t0, t1;
	gettimeofday(&t0, NULL);
	for (pchunk = h->start;
		 pchunk < end;
		 pchunk += step) {
		insertChunkToFL(s, h, pchunk);
	}
	gettimeofday(&t1, NULL);
	long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

	assert(pchunk < h->end);
	assert(h->fl_tail > h->fl_head);

	h->limit = pchunk;


	/*Total number of Chunks in the Chunked heap*/
	s->maxChunksAvailable = s->fl_chunks;

	/*Heuristic chunks = 30% of total chunks created on the chunked heap*/
	s->heuristicChunks = (size_t)(s->hPercent * s->maxChunksAvailable);

#ifdef PROFILE_UMGC
	fprintf(stderr, "[GC] Created heap of %d chunks\n", s->fl_chunks);
#endif

	if (DEBUG or s->controls.messages) {
		fprintf(stderr, "[GC] Created heap of %d chunks in %lu us step=%d sz(umchunk)=%d sz(umhdr)=%d\n",
				s->fl_chunks, elapsed,
				step, sizeof(struct GC_UM_Chunk), sizeof(UM_header));
		fprintf(stderr,
				"[GC: Created heap at "FMTPTR
		" of size %s bytes\n",
				(uintptr_t)(h->start),
				uintmaxToCommaString(h->size));
		fprintf(stderr,
				"[GC: mapped freelist over the heap\n]");
	}


	return TRUE;
}
