void printObjptr(GC_state s, objptr* opp) {
    fprintf(stdout, FMTPTR", ", *opp);
}




void dumpUMHeap(GC_state s) {
    FILE *__j __attribute__ ((unused)) = freopen("heap_ref.txt", "a", stdout);

    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk) + sizeof(UM_header); /*account for size of chunk type header*/
    //pointer end = s->umheap.start + s->umheap.size - step;

    for (pchunk=s->umheap.start;
         pchunk < s->umheap.limit;
         pchunk += step) {
        if(((UM_Mem_Chunk)pchunk)->chunkType == UM_NORMAL_CHUNK)
        {

            GC_UM_Chunk pc = (GC_UM_Chunk)(pchunk+sizeof(UM_header));
            if (pc->chunk_header & UM_CHUNK_IN_USE) {
                fprintf(stdout, "Chunk: %p, Normal: %p -> ", (void*)pchunk, (void*)pc);
                //foreachObjptrInObject(s, pchunk + 4, printObjptr, false);
                fprintf(stdout, "\n");
            }
        }
        else if(((UM_Mem_Chunk)pchunk)->chunkType == UM_ARRAY_CHUNK)
        {

            GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)(pchunk+sizeof(UM_header));
            if (pc->array_chunk_header & UM_CHUNK_IN_USE) {
                fprintf(stdout, "Chunk: %p, Array Node: %p -> ", (void*)pchunk, (void*)pc);
                //foreachObjptrInObject(s, pchunk + 8, printObjptr, false);
                fprintf(stdout, "\n");
            }
        }
        else if((((UM_Mem_Chunk)pchunk)->chunkType == UM_EMPTY))
        {
                fprintf(stdout, "Empty Chunk: %p -> ", (void*)pchunk);
                //foreachObjptrInObject(s, pchunk + 4, printObjptr, false);
                fprintf(stdout, "\n");
        }
                

    }

    fprintf(stdout, "========= DONE =========\n");
    /*fprintf(stdout, "========= ARRAY =========\n");
    step = sizeof(struct GC_UM_Array_Chunk);
    end = s->umheap.start + s->umheap.size - step;

    //    if (s->umheap.fl_array_chunks <= 2000) {
    for (pchunk=s->umheap.start;
         pchunk < end;
         pchunk += step) {
        if(((UM_Mem_Chunk)pchunk)->chunkType == UM_ARRAY_CHUNK)
        {

            GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)pchunk;
            if (pc->array_chunk_header & UM_CHUNK_IN_USE) {
                fprintf(stdout, "Array: "FMTPTR" , "FMTPTR" -> ", (uintptr_t)pchunk, (uintptr_t)(pchunk + 8));
                foreachObjptrInObject(s, pchunk + 8, printObjptr, false);
                fprintf(stdout, "\n");
            }
        }
    }
    */

  /*  fprintf(stdout, "========= STACK =========\n");
    fprintf(stdout, "Stack: ");
    foreachObjptrInObject(s, (pointer) getStackCurrent(s), printObjptr, FALSE);
    fprintf(stdout, "\n");
*/
    fclose(stdout);
}
