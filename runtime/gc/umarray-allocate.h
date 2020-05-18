#if (defined (MLTON_GC_INTERNAL_FUNCS))
GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       GC_UM_Array_Chunk* allocHead,
                                       size_t num,
                                       size_t fan_out);

__attribute__((unused))
GC_UM_Array_Chunk UM_Group_Array_Chunk_new(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       GC_UM_Array_Chunk* allocHead,
                                       size_t num,
                                       size_t fan_out);

pointer UM_walk_array_leafs(pointer _c, size_t *nels);

#endif
