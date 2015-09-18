void initUMHeap(__attribute__ ((unused)) GC_state s,
                GC_UM_heap h) {
    h->start = NULL;
    h->size = 0;
    h->fl_head = NULL;
    h->fl_chunks = 0;
}

void initUMArrayHeap(__attribute__ ((unused)) GC_state s,
                     GC_UM_Array_heap h) {
    h->start = NULL;
    h->size = 0;
    h->fl_array_chunks = 0;
    h->fl_array_head = NULL;
}

GC_UM_Chunk allocNextChunk(GC_state s,
                           GC_UM_heap h) {

    if (h->fl_chunks <= 3) {
//        GC_collect(s, 0, true);
        die("allocNextChunk: No more memory available\n");
    }

    GC_UM_Chunk c = h->fl_head;
    h->fl_head = h->fl_head->next_chunk;
    c->next_chunk = NULL;
    c->chunk_header = UM_CHUNK_HEADER_CLEAN;
    h->fl_chunks -= 1;
    return c;
}

GC_UM_Array_Chunk allocNextArrayChunk(GC_state s,
                                      GC_UM_Array_heap h) {
    if (h->fl_array_chunks <= 3) {
        die("allocNextArrayChunk: No more memory available\n");
    }

    GC_UM_Array_Chunk c = h->fl_array_head;
    h->fl_array_head = h->fl_array_head->next_chunk;
    c->next_chunk = NULL;
    c->array_chunk_magic = 9998;
    c->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
    int i;
    for (i=0; i<UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
        c->ml_array_payload.um_array_pointers[i] = NULL;
    }
    h->fl_array_chunks -= 1;
    return c;
}

void insertFreeChunk(__attribute__ ((unused)) GC_state s,
                     GC_UM_heap h,
                     pointer c) {
    GC_UM_Chunk pc = (GC_UM_Chunk) c;
    //    memset(pc->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE);
    pc->next_chunk = h->fl_head;
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header = UM_CHUNK_HEADER_CLEAN;
    h->fl_head = pc;
    h->fl_chunks += 1;
}

void insertArrayFreeChunk(__attribute__ ((unused)) GC_state s,
                          GC_UM_Array_heap h,
                          pointer c) {
    GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk) c;
    //    memset(pc->ml_array_payload.ml_object, 0, UM_CHUNK_ARRAY_PAYLOAD_SIZE);
    pc->next_chunk = h->fl_array_head;
    pc->array_chunk_header = UM_CHUNK_HEADER_CLEAN;
    h->fl_array_head = pc;
    h->fl_array_chunks += 1;
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
    size_t step = sizeof(struct GC_UM_Chunk);
    pointer end = h->start + h->size - step;


    for (pchunk=h->start;
         pchunk < end;
         pchunk+=step) {
        insertFreeChunk(s, h, pchunk);
    }

#ifdef PROFILE_UMGC
    fprintf(stderr, "[GC] Created heap of %d chunks\n", h->fl_chunks);
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

bool createUMArrayHeap(__attribute__ ((unused)) GC_state s,
                       GC_UM_Array_heap h,
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
        insertArrayFreeChunk(s, h, pchunk);
    }

#ifdef PROFILE_UMGC
    fprintf(stderr, "[GC] Created array heap of %d chunks\n", h->fl_array_chunks);
#endif

    return TRUE;
}
