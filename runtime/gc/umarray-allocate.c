

int getLengthOfList(GC_UM_Array_Chunk head)
{
    GC_UM_Array_Chunk current = head;
    int i = 0;
    while(current!=NULL)
    {
        current = current->next_chunk;
        i++;

    }

    return i;
}





pointer GC_arrayAllocate (GC_state s,
                          __attribute__ ((unused)) size_t ensureBytesFree,
                          GC_arrayLength numElements,
                          GC_header header) {
//    size_t arraySize;
    size_t bytesPerElement =0;
    uint16_t bytesNonObjptrs=0;
    uint16_t numObjptrs = 0;
//    pointer frontier, last;
//    pointer result;

//    GC_collect(s, 0, false);
//    GC_collect(s, 0, false);
    splitHeader(s, header, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
    bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);

    size_t chunkNumObjs = UM_CHUNK_ARRAY_PAYLOAD_SIZE / bytesPerElement;
    size_t numChunks = numElements / chunkNumObjs + (numElements % chunkNumObjs != 0);
    

    if (DEBUG_MEM) {
        fprintf(stderr, "numElements: %d, chunkNumObjs: %d, numChunks: %d\n",
                numElements, chunkNumObjs, numChunks);
    }
        
    /* Calculate number of chunks the array representation
     * is going ot use. 
     * numChunksToRequest = numChunks for leaf + number of chunks for 1st group internal nodes + .... + 1 chunk for root node*/    
    size_t numChunksToRequest = numChunks;
    int x = numChunks;
    while(x != 1 && numChunksToRequest > 1)
    {
        if(x < 32)
            x = 1;
        else
            x= 1+ ((x-1)/UM_CHUNK_ARRAY_INTERNAL_POINTERS);

        numChunksToRequest += x;
    }

    assert(numChunksToRequest >= numChunks);

   
    /*Will block if there aren't enough chunks*/    
    reserveAllocation(s,numChunksToRequest);

    GC_UM_Array_Chunk allocHead = allocateArrayChunks(s, &(s->umheap),numChunksToRequest);
    
    if(DEBUG_CHUNK_ARRAY)
        fprintf(stderr,"%d] Initial allocHead length: %d, numChunks = %d\n",PTHREAD_NUM,getLengthOfList(allocHead),numChunks);

    GC_UM_Array_Chunk new = allocHead;
    allocHead = allocHead->next_chunk;
    new->next_chunk = NULL;

    GC_UM_Array_Chunk parray_header = new;
    

    parray_header->array_chunk_counter = 0;
    parray_header->array_chunk_length = numElements;
    parray_header->array_chunk_ml_header = header;
    parray_header->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
    parray_header->array_chunk_numObjs = chunkNumObjs;
    parray_header->array_num_chunks = numChunks;
    parray_header->array_chunk_objSize = bytesPerElement;
    parray_header->parent = NULL;
    parray_header->array_chunk_header |= UM_CHUNK_IN_USE;

    if (numChunks <= 1 || numElements == 0) {
        return (pointer)&(parray_header->ml_array_payload.ml_object[0]);
    }

    GC_UM_Array_Chunk cur_chunk = parray_header;
    int i;

    for (i=0; i<numChunks - 1; i++) {
        
        assert(allocHead != NULL);
        GC_UM_Array_Chunk tmp = allocHead;
        allocHead = allocHead->next_chunk;
        tmp->next_chunk = NULL;

        cur_chunk->next_chunk = tmp;
        cur_chunk->next_chunk->array_chunk_fan_out = chunkNumObjs;
        cur_chunk = cur_chunk->next_chunk;
        cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
        cur_chunk->array_chunk_header |= UM_CHUNK_IN_USE;
    }

    if(DEBUG_CHUNK_ARRAY)
        fprintf(stderr,"%d] allocHead length after Leaf alloc: %d\n",PTHREAD_NUM,getLengthOfList(allocHead));

    GC_UM_Array_Chunk root = UM_Group_Array_Chunk(s,
                                                  parray_header,
                                                  &allocHead,
                                                  UM_CHUNK_ARRAY_INTERNAL_POINTERS,
                                                  1);
                                                  //chunkNumObjs);


    if(DEBUG_CHUNK_ARRAY)
    {
       fprintf(stderr,"%d] allocHead length after 1st group alloc: %d\n",PTHREAD_NUM,getLengthOfList(allocHead));
       fprintf(stderr, "%d] 1st group created array with chunk_fan_out: %d\n",PTHREAD_NUM,root->array_chunk_fan_out);
    }

    while (root->next_chunk) {
        root = UM_Group_Array_Chunk(s, root,&allocHead, UM_CHUNK_ARRAY_INTERNAL_POINTERS,
                                    root->array_chunk_fan_out * UM_CHUNK_ARRAY_INTERNAL_POINTERS);
    }

    if(DEBUG_CHUNK_ARRAY)
        fprintf(stderr,"%d] allocHead length after all alloc: %d\n",PTHREAD_NUM,getLengthOfList(allocHead));

    root->array_chunk_numObjs = parray_header->array_chunk_numObjs;
    root->array_chunk_length = parray_header->array_chunk_length;
    parray_header->root = root;

    if (DEBUG_CHUNK_ARRAY)
        fprintf(stderr, "%d] Created array with chunk_fan_out: %d\n",PTHREAD_NUM,root->array_chunk_fan_out);

    /*ensure that no chunks remain in the initially allocated list*/
    assert(allocHead == NULL);

    return (pointer)&(parray_header->ml_array_payload);
}

GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       GC_UM_Array_Chunk* allocHead,
                                       size_t num,
                                       size_t fan_out)
{
    if (head->next_chunk == NULL)
        return head;

    assert(*allocHead != NULL);
    
    GC_UM_Array_Chunk new = *allocHead;
    *allocHead = (*allocHead)->next_chunk;
    new->next_chunk = NULL;
    
    GC_UM_Array_Chunk start = new;

    GC_UM_Array_Chunk cur_chunk = start;
    cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
    cur_chunk->array_chunk_header |= UM_CHUNK_IN_USE;
    cur_chunk->array_chunk_fan_out = fan_out;

    int cur_index = 0;
    while (head) {
        cur_chunk->ml_array_payload.um_array_pointers[cur_index] = head;
        head = head->next_chunk;
        cur_index++;
        if (cur_index >= num && head) {
            assert(*allocHead != NULL);
            
            GC_UM_Array_Chunk tmp = *allocHead;
            *allocHead = (*allocHead)->next_chunk;
            tmp->next_chunk = NULL;
            
            cur_chunk->next_chunk = tmp;

            cur_chunk = cur_chunk->next_chunk;
            cur_chunk->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
            cur_chunk->array_chunk_header |= UM_CHUNK_IN_USE;
            cur_chunk->array_chunk_fan_out = fan_out;
            cur_index = 0;
        }
    }

    return start;
}
