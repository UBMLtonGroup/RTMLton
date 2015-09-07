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

    if (DEBUG_MEM) {
        fprintf(stderr, "numElements: %d, chunkNumObjs: %d, numChunks: %d\n",
                numElements, chunkNumObjs, numChunks);
    }

    GC_UM_Array_Chunk parray_header = allocNextArrayChunk(s, &s->umarheap);
    parray_header->array_chunk_counter = 0;
    parray_header->array_chunk_length = numElements;
    parray_header->array_chunk_ml_header = header;
    parray_header->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
    parray_header->array_chunk_numObjs = chunkNumObjs;
    parray_header->array_num_chunks = numChunks;
    parray_header->array_chunk_objSize = bytesPerElement;
    parray_header->parent = NULL;
    parray_header->array_chunk_header = UM_CHUNK_IN_USE;

    if (numChunks <= 1) {
        return (pointer)&(parray_header->ml_array_payload);
    }

    GC_UM_Array_Chunk cur_chunk = parray_header;
    int i;

    for (i=0; i<numChunks - 1; i++) {
        cur_chunk->next_chunk = allocNextArrayChunk(s, &s->umarheap);
        cur_chunk->next_chunk->array_chunk_fan_out = chunkNumObjs;
        cur_chunk = cur_chunk->next_chunk;
        cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
    }

    GC_UM_Array_Chunk root = UM_Group_Array_Chunk(s,
                                                  parray_header,
                                                  UM_CHUNK_ARRAY_INTERNAL_POINTERS,
                                                  1);
                                                  //chunkNumObjs);

    if (DEBUG_MEM)
        fprintf(stderr, "1st group created array with chunk_fan_out: %d\n",
                root->array_chunk_fan_out);

    while (root->next_chunk) {
        root = UM_Group_Array_Chunk(s, root, UM_CHUNK_ARRAY_INTERNAL_POINTERS,
                                    root->array_chunk_fan_out *
                                    UM_CHUNK_ARRAY_INTERNAL_POINTERS);
    }

    root->array_chunk_numObjs = parray_header->array_chunk_numObjs;
    parray_header->root = root;

    if (DEBUG_MEM)
        fprintf(stderr, "Created array with chunk_fan_out: %d\n",
                root->array_chunk_fan_out);
    return (pointer)&(parray_header->ml_array_payload);
}

GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       size_t num,
                                       size_t fan_out)
{
    if (head->next_chunk == NULL)
        return head;

    GC_UM_Array_Chunk start = allocNextArrayChunk(s, &s->umarheap);
    GC_UM_Array_Chunk cur_chunk = start;
    cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
    cur_chunk->array_chunk_header = UM_CHUNK_IN_USE;
    cur_chunk->array_chunk_fan_out = fan_out;

    int cur_index = 0;
    while (head) {
        cur_chunk->ml_array_payload.um_array_pointers[cur_index] = head;
        head = head->next_chunk;
        cur_index++;
        if (cur_index >= num && head) {
            cur_chunk->next_chunk = allocNextArrayChunk(s, &s->umarheap);
            cur_chunk = cur_chunk->next_chunk;
            cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
            cur_chunk->array_chunk_header = UM_CHUNK_IN_USE;
            cur_chunk->array_chunk_fan_out = fan_out;
            cur_index = 0;
        }
    }

    return start;
}
