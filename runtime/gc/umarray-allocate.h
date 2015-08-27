#if (defined (MLTON_GC_INTERNAL_FUNCS))
static size_t UM_Create_Array_Chunk(GC_state s,
                                    GC_UM_Array_heap h,
                                    GC_UM_Array_Chunk root,
                                    size_t numChunks,
                                    size_t height);

GC_UM_Array_Chunk UM_Group_Array_Chunk(GC_state s,
                                       GC_UM_Array_Chunk head,
                                       size_t num,
                                       size_t fan_out);
#endif
