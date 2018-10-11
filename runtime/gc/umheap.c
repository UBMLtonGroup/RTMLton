


#define IFED(X) do { if (X) { perror("perror " #X); exit(-1); } } while(0)

static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

#define A_LOCK IFED(pthread_mutex_lock(&alloc_lock))
#define A_UNLOCK IFED(pthread_mutex_unlock(&alloc_lock))


void initUMHeap(GC_state s,
                GC_UM_heap h) {
    h->start = NULL;
    h->size = 0;
    h->fl_head = NULL;
    s->fl_chunks = 0;
}


static
GC_UM_Chunk initializeChunk(GC_state s, GC_UM_heap h, pointer c) {

    GC_UM_Chunk pc = (GC_UM_Chunk) c;
    memset(pc->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE);
    pc->next_chunk = NULL;
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header = UM_CHUNK_HEADER_CLEAN;
    //h->fl_head = pc;
    // s->fl_chunks += 1;
    return pc;
}

GC_UM_Chunk allocNextChunk(GC_state s,
                           GC_UM_heap h) {

    A_LOCK;
    if (s->fl_chunks <= 3) {
//        GC_collect(s, 0, true);
        die("allocNextChunk: No more memory available\n");
    }
    h->fl_head->chunkType = UM_NORMAL_CHUNK;
    struct UM_Mem_Chunk *nc = h->fl_head->next_chunk;
    GC_UM_Chunk c = initializeChunk(s, h, ((pointer) h->fl_head + 4)); /*pass pointer to area after chunktype*/
    memset(c->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE + UM_CHUNK_PAYLOAD_SAFE_REGION);
    h->fl_head = nc;
    c->next_chunk = NULL;
    c->chunk_header = UM_CHUNK_HEADER_CLEAN;
    c->ra = 0;
    s->fl_chunks -= 1;
    A_UNLOCK;
    return c;
}


static
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

GC_UM_Array_Chunk allocNextArrayChunk(GC_state s,
                                      GC_UM_heap h) {
    A_LOCK;
    if (s->fl_chunks <= 0) {
        die("allocNextArrayChunk: No more memory available\n");
    }
    h->fl_head->chunkType = UM_ARRAY_CHUNK;
    struct UM_Mem_Chunk *nc = h->fl_head->next_chunk;
    GC_UM_Array_Chunk c = insertArrayFreeChunk(s, h,
                                               ((pointer) h->fl_head + 4)); /*pass pointer to area after chunktype*/
    h->fl_head = nc;
    c->next_chunk = NULL;
    c->array_chunk_magic = 9998;
    c->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
    int i;
    for (i = 0; i < UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
        c->ml_array_payload.um_array_pointers[i] = NULL;
    }
    s->fl_chunks -= 1;
    A_UNLOCK;
    return c;
}

/* called from garbage collector so locking is needed */

void insertFreeChunk(GC_state s,
                     GC_UM_heap h,
                     pointer c) {
    A_LOCK;
    UM_Mem_Chunk pc = (UM_Mem_Chunk) c;
    pc->next_chunk = h->fl_head;
    pc->chunkType = UM_EMPTY;
    h->fl_head = pc;
    s->fl_chunks += 1;
    A_UNLOCK;
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

    fprintf(stderr, "creating UMHeap with chunkSize of %u\n", sizeof(struct GC_UM_Chunk));

    h->start = newStart;
    h->size = desiredSize;
    h->end = newStart + desiredSize;

    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk) +
                  sizeof(Word32_t);/*account for size of chunktype field*/ //TODO: reason if it should be sizeof(struct GC_UM_Chunk) + sizeof(struct UM_MEM_Chunk)
    pointer end = h->start + h->size - step;


    for (pchunk = h->start;
         pchunk < end;
         pchunk += step) {
        insertFreeChunk(s, h, pchunk);
    }

#ifdef PROFILE_UMGC
    fprintf(stderr, "[GC] Created heap of %d chunks\n", s->fl_chunks);
#endif

    if (DEBUG or s->controls.messages) {
        fprintf(stderr,
                "[GC: Created heap at "
        FMTPTR
        " of size %s bytes\n",
                (uintptr_t)(h->start),
                uintmaxToCommaString(h->size));
        fprintf(stderr,
                "[GC: mapped freelist over the heap\n]");
    }

    return TRUE;
}
