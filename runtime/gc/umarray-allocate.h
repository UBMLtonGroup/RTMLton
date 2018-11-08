#if (defined (MLTON_GC_INTERNAL_FUNCS))
int getLengthOfList(GC_UM_Array_Chunk head);
GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       GC_UM_Array_Chunk* allocHead,
                                       size_t num,
                                       size_t fan_out);
#endif
