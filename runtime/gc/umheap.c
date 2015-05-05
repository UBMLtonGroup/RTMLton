void initUMHeap(__attribute__ ((unused)) GC_state s,
                GC_UM_heap h) {
    h->start = NULL;
    h->size = 0;
    h->fl_head = NULL;
    h->fl_chunks = 0;
}

GC_UM_Chunk allocNextChunk(__attribute__ ((unused)) GC_state s,
                           GC_UM_heap h) {

    if (h->fl_chunks <= 3) {
        die("allocNextChunk: No more memory available\n");
    }

    GC_UM_Chunk c = h->fl_head;
    h->fl_head = h->fl_head->next_chunk;
    c->next_chunk = NULL;
    c->chunk_header = UM_CHUNK_HEADER_CLEAN;
    h->fl_chunks -= 1;
    return c;
}

void insertFreeChunk(__attribute__ ((unused)) GC_state s,
                     GC_UM_heap h,
                     pointer c) {
    GC_UM_Chunk pc = (GC_UM_Chunk) c;
    memset(pc->ml_object, 0, UM_CHUNK_PAYLOAD_SIZE);
    pc->next_chunk = h->fl_head;
    pc->sentinel = UM_CHUNK_SENTINEL_UNUSED;
    pc->chunk_header = UM_CHUNK_HEADER_CLEAN;
    h->fl_head = pc;
    h->fl_chunks += 1;
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

    pointer pchunk;
    pointer end = h->start + h->size;
    size_t step = sizeof(struct GC_UM_Chunk);

    for (pchunk=h->start;
         pchunk < end;
         pchunk+=step) {
        insertFreeChunk(s, h, pchunk);
    }

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

    return TRUE;
}
