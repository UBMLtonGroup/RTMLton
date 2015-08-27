size_t UM_Create_Array_Chunk(GC_state s,
                             GC_UM_Array_heap h,
                             GC_UM_Array_Chunk root,
                             size_t numChunks,
                             size_t height) {
    size_t i;
    root->array_height = height;
    root->array_chunk_fan_out = (1 << (5 * height)); // 32 ^ height
    if (height == 0) {
        root->array_chunk_fan_out = 1;
        size_t goal = (numChunks <= UM_CHUNK_ARRAY_INTERNAL_POINTERS) ? numChunks :
            UM_CHUNK_ARRAY_INTERNAL_POINTERS;

        for (i=0; i<goal; i++) {
            GC_UM_Array_Chunk puac = allocNextArrayChunk(s, &s->umarheap);
            puac->array_chunk_header = UM_CHUNK_IN_USE;
            puac->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
            puac->parent = root;
            root->ml_array_payload.um_array_pointers[i] = puac;
        }
        return goal;
    }

    i = 0;
    size_t chunk_processed = 0;
    for (i=0; i<UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
        GC_UM_Array_Chunk puac = allocNextArrayChunk(s, &s->umarheap);
        puac->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
        puac->array_chunk_header = UM_CHUNK_IN_USE;
        puac->parent = root;
        root->ml_array_payload.um_array_pointers[i] = puac;
        size_t t = UM_Create_Array_Chunk(
            s, h, puac, numChunks, height - 1);
        chunk_processed += t;
        if (chunk_processed >= numChunks) {
            break;
        }
    }

    return chunk_processed;
}

pointer GC_arrayAllocate (GC_state s,
                          __attribute__ ((unused)) size_t ensureBytesFree,
                          GC_arrayLength numElements,
                          GC_header header) {
//    size_t arraySize;
    size_t bytesPerElement;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
//    pointer frontier, last;
//    pointer result;

//    GC_collect(s, 0, false);
    GC_collect(s, 0, false);
    splitHeader(s, header, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
    bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);

    size_t chunkNumObjs = UM_CHUNK_ARRAY_PAYLOAD_SIZE / bytesPerElement;
    size_t numChunks = numElements / chunkNumObjs + (numElements % chunkNumObjs != 0);
    size_t height = (size_t) ceil(log(numChunks) / log(UM_CHUNK_ARRAY_INTERNAL_POINTERS));
    if (DEBUG_MEM) {
        fprintf(stderr, "numElements: %d, chunkNumObjs: %d, numChunks: %d, height: %d"
                "log numChunks: %f, log internal pointers: %f\n",
                numElements, chunkNumObjs, numChunks, height,
                log(numChunks), log(UM_CHUNK_ARRAY_INTERNAL_POINTERS));
    }

    GC_UM_Array_Chunk parray_header = allocNextArrayChunk(s, &s->umarheap);
    parray_header->array_chunk_counter = 0;
    parray_header->array_chunk_length = numElements;
    parray_header->array_chunk_ml_header = header;
    parray_header->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
    parray_header->array_chunk_numObjs = chunkNumObjs;
    parray_header->array_num_chunks = numChunks;
    parray_header->array_chunk_objSize = bytesPerElement;
    parray_header->parent = NULL;
    parray_header->array_chunk_header = UM_CHUNK_IN_USE;

    if (numChunks == 0) {
        return (pointer)&(parray_header->ml_array_payload);
    }

    GC_UM_Array_Chunk cur_chunk = parray_header;
    int i;

    for (i=0; i<numChunks; i++) {
      cur_chunk->next = allocateNextArrayChunks(s, &s->umarheap);
      cur_chunk = cur_chunk->next_chunk;
    }

    GC_UM_Array_Chunk root = UM_Group_Array_Chunk(s,
                                                  parray_header->next_chunk,
                                                  UM_CHUNK_ARRAY_INTERNAL_POINTERS);
    while (root->next) {
      root = UM_Group_Array_Chunk(s, root, UM_CHUNK_ARRAY_INTERNAL_POINTERS);
    }

    // TODO: Add pointer to root for the header / 2nd chunk
    /* Find the left most leaf, return its payload as the pointer for
       relative addressing: a hack to fix string printing.
    */
    GC_UM_Array_Chunk current = parray_header;
    while (current->array_chunk_type != UM_CHUNK_ARRAY_LEAF) {
        current = current->ml_array_payload.um_array_pointers[0];
    }
    current->array_chunk_ml_header = header;
    current->next_chunk = parray_header;
    current->array_chunk_length = numElements;
    current->array_chunk_objSize = bytesPerElement;
    return (pointer) &(current->ml_array_payload); // parray_header->ml_array_payload);

    /* Not used below */
//    if (DEBUG_MEM) {
//        fprintf(stderr, "numElements: %d, chunkNumObjs: %d, numChunks: %d\n",
//                numElements, chunkNumObjs, numChunks);
//    }
//
//    arraySize = bytesPerElement * numElements;
//    arraySize += GC_ARRAY_HEADER_SIZE;
//
//    if (arraySize < GC_ARRAY_HEADER_SIZE + OBJPTR_SIZE) {
//        arraySize = GC_ARRAY_HEADER_SIZE + OBJPTR_SIZE;
//    }
//
//    frontier = s->umarfrontier;
//    last = frontier + arraySize;
//    s->umarfrontier = last;
//
//    if (last >= s->umarheap.start + s->umarheap.size) {
//        die("Array allocate out of memory!\n");
//    }
//
//    *((GC_arrayCounter*)(frontier)) = 0;
//    frontier = frontier + GC_ARRAY_COUNTER_SIZE;
//    *((GC_arrayLength*)(frontier)) = numElements;
//    frontier = frontier + GC_ARRAY_LENGTH_SIZE;
//    *((GC_header*)(frontier)) = header;
//    frontier = frontier + GC_HEADER_SIZE;
//
//    result = frontier;
//
//    if (1 <= numObjptrs and 0 < numElements) {
//        pointer p;
//
//        if (0 == bytesNonObjptrs)
//            for (p = frontier; p < last; p += OBJPTR_SIZE)
//                *((objptr*)p) = BOGUS_OBJPTR;
//        else {
//            /* Array with a mix of pointers and non-pointers. */
//            size_t bytesObjptrs;
//
//            bytesObjptrs = numObjptrs * OBJPTR_SIZE;
//
//            for (p = frontier; p < last; ) {
//                pointer next;
//
//                p += bytesNonObjptrs;
//                next = p + bytesObjptrs;
//                assert (next <= last);
//                for ( ; p < next; p += OBJPTR_SIZE)
//                    *((objptr*)p) = BOGUS_OBJPTR;
//            }
//        }
//    }
//
//    return result;
}

GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       size_t num)
{
  if (head->next_chunk == NULL)
    return head;

  GC_UM_Array_Chunk start = allocNextArrayChunk(s, &s->umarheap);
  GC_UM_Array_Chunk cur_chunk = start;

  int cur_index = 0;
  while (head) {
    cur_chunk->ml_array_payload.ml_object[cur_index] = head;
    head = head->next_chunk;
    cur_index++;
    if (cur_index >= num) {
      cur_chunk->next_chunk = allocNextArrayChunk(s, &s->umarheap);
      cur_chunk = cur_chunk->next_chunk;
      cur_index = 0;
    }
  }

  return start;
}
