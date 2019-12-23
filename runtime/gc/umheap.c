



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
    s->fl_chunks = 0;
	s->maxChunksAvailable = 0;
	s->heuristicChunks = 0;
}

#ifdef STACK_GC_SANITY
extern GC_UM_Chunk stack_list[];
extern unsigned int stack_list_end;
#endif

GC_UM_Chunk insertFreeUMChunk(GC_state s, GC_UM_heap h, pointer c){

    GC_UM_Chunk pc = (GC_UM_Chunk) c;

    pc->next_chunk = NULL; // TODO is this correct? pc is c+offset so struct mapping is invalid now?
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header = UM_CHUNK_HEADER_CLEAN;

    return pc;
}



GC_UM_Chunk allocNextChunk(GC_state s,
                           GC_UM_heap h) {


    /*Only place this should be called is allocChunk to preserve fl_lock*/
    
    /*Allocate next chunk from start of free list*/
    h->fl_head->chunkType= UM_NORMAL_CHUNK;
    struct UM_Mem_Chunk* nc= h->fl_head->next_chunk;
    GC_UM_Chunk c = insertFreeUMChunk(s, h,((pointer)h->fl_head + sizeof(UM_header) )); /*pass pointer to area after chunktype*/

    if(nc == NULL )
    	{
        	h->fl_head = NULL;
        	h->fl_tail = NULL;
    	}
    else
    	{
        	h->fl_head = nc;
    	}
    
    c->next_chunk = c->prev_chunk = NULL; // TODO is this correct? c is pc+offset now
    c->chunk_header = UM_CHUNK_HEADER_CLEAN;
    if( s->rtSync[PTHREAD_NUM])
    {
        c->chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
	}
	else
	{
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

GC_UM_Chunk allocateChunks(GC_state s, GC_UM_heap h,size_t numChunks)
{
    LOCK_FL;

	/*if (s->fl_chunks < 1 || s->fl_chunks < numChunks)
		{
		  blockAllocator(s,numChunks);
		}
	*/

	assert(numChunks <= s->reserved);

    GC_UM_Chunk head = allocNextChunk(s,&(s->umheap));
    head->chunk_header |= UM_CHUNK_IN_USE;
    
    if(numChunks > 1)
    {
        int i;
        GC_UM_Chunk current = head;
        for (i=0; i< (numChunks -1);i++)
        {
            
            current->next_chunk = allocNextChunk(s, &(s->umheap));
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
    struct UM_Mem_Chunk* nc= h->fl_head->next_chunk;
    assert(nc != NULL);
    GC_UM_Array_Chunk c = insertArrayFreeChunk(s, h,((pointer)h->fl_head + sizeof(UM_header) )); /*pass pointer to area after chunktype*/
    
    if(nc == NULL )
    	{
        	h->fl_head = NULL;
        	h->fl_tail = NULL;
    	}
    else
    	{
        	h->fl_head = nc;
    	}
    
    c->next_chunk = NULL;
    c->array_chunk_magic = 9998;
    c->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
    if(s->rtSync[PTHREAD_NUM])
    {
        c->array_chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
    }
    else
    {
        c->array_chunk_header |= UM_CHUNK_RED_MASK;
    }
    int i;
    for (i=0; i<UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
        c->ml_array_payload.um_array_pointers[i] = NULL;
    }
    s->fl_chunks -= 1;
    s->reserved -= 1;
    s->cGCStats.numChunksAllocated++;
    return c;
}

GC_UM_Array_Chunk allocateArrayChunks(GC_state s,GC_UM_heap h,size_t numChunks)
{

   LOCK_FL;
	/*if (s->fl_chunks < 1 || s->fl_chunks < numChunks) {

		 blockAllocator(s,numChunks);
	 }*/

	assert(numChunks <= s->reserved);
    
    GC_UM_Array_Chunk head = allocNextArrayChunk(s,&(s->umheap));
    
    if(numChunks > 1)
    {
        int i;
        GC_UM_Array_Chunk current = head;
        for (i=0; i< (numChunks -1);i++)
        {
            
            current->next_chunk = allocNextArrayChunk(s, &(s->umheap));
            current = current->next_chunk;
        }
    }
    
    UNLOCK_FL;

    return head;
    
    
    
}



void insertFreeChunk(GC_state s,
                     GC_UM_heap h,
                     pointer c) {
    LOCK_FL;

    /*Insert free chunk to back of free list*/
    UM_Mem_Chunk pc = (UM_Mem_Chunk)c;
    if(s->fl_chunks == 0)
    {
        h->fl_head= pc;
        h->fl_tail = pc;
        pc->chunkType = UM_EMPTY;
        pc->next_chunk = NULL;
        s->fl_chunks +=1;
    }
    else
    {
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
			if (stack_list[i] == ((GC_UM_Chunk) pc + sizeof(UM_header))) {
				fprintf(stderr, RED("stack frame added to free list\n"));
				die("abort");
			}
		}
	}
#endif
    UNLOCK_FL;
    
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
    newStart = GC_mmapAnon (NULL, desiredSize);;

    if (newStart == (void*) -1) {
        fprintf(stderr, "[GC: MMap Failure]\n");
        return FALSE;
    }

    h->start = newStart;
    h->size = desiredSize;
    h->end = newStart + desiredSize;

    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk) + sizeof(UM_header); /*account for size of chunktype field*/
    pointer end = h->start + h->size - step;

    struct timeval t0, t1;
	gettimeofday(&t0, NULL);
    for (pchunk=h->start;
         pchunk < end;
         pchunk+=step) {
        insertFreeChunk(s, h, pchunk);
    }
	gettimeofday(&t1, NULL);
	long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;

    assert(pchunk < h->end);
    assert(h->fl_tail > h->fl_head);
    
    h->limit = pchunk;


    /*Total number of Chunks in the Chunked heap*/
    s->maxChunksAvailable = s->fl_chunks;

	/*Heuristic chunks = 30% of total chunks created on the chunked heap*/
	s->heuristicChunks =(size_t)( s->hPercent * s->maxChunksAvailable );

#ifdef PROFILE_UMGC
	fprintf(stderr, "[GC] Created heap of %d chunks\n", s->fl_chunks);
#endif

    if (DEBUG or s->controls.messages) {
		fprintf(stderr, "[GC] Created heap of %d chunks in %lu us step=%d sz(umchunk)=%d sz(umhdr)=%d\n",
				s->fl_chunks, elapsed,
				step, sizeof(struct GC_UM_Chunk), sizeof(UM_header));
        fprintf (stderr,
                 "[GC: Created heap at "FMTPTR" of size %s bytes\n",
                 (uintptr_t)(h->start),
                 uintmaxToCommaString(h->size));
        fprintf(stderr,
                "[GC: mapped freelist over the heap\n]");
    }


    

    return TRUE;
}

/*bool createUMArrayHeap(__attribute__ ((unused)) GC_state s,
                       GC_UM_heap h,
                       size_t desiredSize,
                       __attribute__ ((unused)) size_t minSize) {
    pointer newStart;
    newStart = GC_mmapAnon (NULL, desiredSize);;

    if (newStart == (void*) -1) {
        fprintf(stderr, "[GC: MMap Failure]\n");
        return FALSE;
    }



    h->start = newStart;
    h->size = desiredSize;

    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Array_Chunk);
    pointer end = h->start + h->size - step;

    for (pchunk=h->start;
         pchunk < end;
         pchunk+=step) {
        insertFreeChunk(s, h, pchunk);
    }

#ifdef PROFILE_UMGC
    fprintf(stderr, "[GC] Created array heap of %d chunks\n", s->fl_chunks);
#endif

    return TRUE;
}*/
