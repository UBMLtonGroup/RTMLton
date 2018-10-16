



#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)
#define LOCK IFED(pthread_mutex_lock(&s->fl_lock))
#define UNLOCK IFED(pthread_mutex_unlock(&s->fl_lock))

#define BLOCK IFED(pthread_cond_wait(&s->fl_empty_cond,&s->fl_lock))


void initUMHeap(GC_state s,
                GC_UM_heap h) {
    h->start = NULL;
    h->size = 0;
    h->fl_head = NULL;
    h->fl_tail = NULL;
    s->fl_chunks = 0;
}

/*void initUMArrayHeap(GC_state s,
                     GC_UM_heap h) {
    h->start = NULL;
    h->size = 0;
    s->fl_chunks = 0;
    h->fl_head = NULL;
}*/

GC_UM_Chunk insertFreeUMChunk(GC_state s, GC_UM_heap h, pointer c){

    GC_UM_Chunk pc = (GC_UM_Chunk) c;
    //    memset(pc->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE);
    pc->next_chunk = NULL;
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header |= UM_CHUNK_HEADER_CLEAN;
    //h->fl_head = pc;
   // s->fl_chunks += 1;
   return pc;
}

GC_UM_Chunk allocNextChunk(GC_state s,
                           GC_UM_heap h) {


    LOCK;

    if (s->fl_chunks <= 3) 
    {
        if(DEBUG_RTGC)
            fprintf(stderr,"%d] Going to block for GC, FC =%d\n",PTHREAD_NUM,s->fl_chunks);
      
        /*Blocks on cond variable , automatically unlocks s->fl_lock*/
        BLOCK;

        if(DEBUG_RTGC)
            fprintf(stderr,"%d] Back from waiting for GC to clear chunks, FC =%d\n",PTHREAD_NUM,s->fl_chunks);
        
        if(s->fl_chunks < 3)
            die("allocNextChunk: No more memory available\n");

    }

        /*Allocate next chunk from start of free list*/
    h->fl_head->chunkType= UM_NORMAL_CHUNK;
    struct UM_Mem_Chunk* nc= h->fl_head->next_chunk;
    GC_UM_Chunk c = insertFreeUMChunk(s, h,((pointer)h->fl_head +4 )); /*pass pointer to area after chunktype*/
    h->fl_head = nc;
    c->next_chunk = NULL;
    c->chunk_header |= UM_CHUNK_HEADER_CLEAN;
    s->fl_chunks -= 1;
    s->cGCStats.numChunksAllocated++;
    UNLOCK;
    return c;
}

GC_UM_Array_Chunk allocNextArrayChunk(GC_state s,
                                      GC_UM_heap h) {
   
   LOCK; 
    if (s->fl_chunks <= 3) {
       
        if(DEBUG_RTGC)
           fprintf(stderr,"%d] Going to block for GC, FC =%d\n",PTHREAD_NUM,s->fl_chunks);
      /*Blocks on cond variable , autamatically unlocks s->fl_lock*/
       BLOCK;
       
      if(DEBUG_RTGC)
          fprintf(stderr,"%d] Back from waiting for GC to clear chunks, FC =%d\n",PTHREAD_NUM,s->fl_chunks);
       
      if(s->fl_chunks < 3)
            die("allocNextChunk: No more memory available\n");

       // die("allocNextArrayChunk: No more memory available\n");
    }

    /*Allocate next chunk from start of list*/
    h->fl_head->chunkType = UM_ARRAY_CHUNK;
    struct UM_Mem_Chunk* nc= h->fl_head->next_chunk;
    GC_UM_Array_Chunk c = insertArrayFreeChunk(s, h,((pointer)h->fl_head + 4)); /*pass pointer to area after chunktype*/
    h->fl_head = nc;
    c->next_chunk = NULL;
    c->array_chunk_magic = 9998;
    c->array_chunk_header |= UM_CHUNK_HEADER_CLEAN;
    int i;
    for (i=0; i<UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
        c->ml_array_payload.um_array_pointers[i] = NULL;
    }
    s->fl_chunks -= 1;
    s->cGCStats.numChunksAllocated++;
    UNLOCK;
    return c;
}

void insertFreeChunk(GC_state s,
                     GC_UM_heap h,
                     pointer c) {
    /*GC_UM_Chunk pc = (GC_UM_Chunk) c;
    //    memset(pc->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE);
    pc->next_chunk = h->fl_head;
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header |= UM_CHUNK_HEADER_CLEAN;
    h->fl_head = pc;
    s->fl_chunks += 1;*/
   

    LOCK;

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

    UNLOCK;
    
}
/*void insertFreeChunkArr(GC_state s,
                     GC_UM_heap h,
                     pointer c) {

    UM_Mem_Chunk pc = (UM_Mem_Chunk)c;
    pc->next_chunk = h->fl_head;
    h->fl_head = pc;
    s->fl_chunks += 1;
    
}*/


GC_UM_Array_Chunk insertArrayFreeChunk(GC_state s,
                          GC_UM_heap h,
                          pointer c) {

   
    GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk) c;
    //    memset(pc->ml_array_payload.ml_object, 0, UM_CHUNK_ARRAY_PAYLOAD_SIZE);
    pc->next_chunk = NULL;
    pc->array_chunk_header |= UM_CHUNK_HEADER_CLEAN;
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
    size_t step = sizeof(struct GC_UM_Chunk) + sizeof(Word32_t);/*account for size of chunktype field*/ //TODO: reason if it should be sizeof(struct GC_UM_Chunk) + sizeof(struct UM_MEM_Chunk)
    pointer end = h->start + h->size - step;


    for (pchunk=h->start;
         pchunk < end;
         pchunk+=step) {
        insertFreeChunk(s, h, pchunk);
    }

    /*Total number of Chunks in the Chunked heap*/
    s->maxChunksAvailable = s->fl_chunks;

#ifdef PROFILE_UMGC
    fprintf(stderr, "[GC] Created heap of %d chunks\n", s->fl_chunks);
#endif

    if (DEBUG or s->controls.messages) {
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
