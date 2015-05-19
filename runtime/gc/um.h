
Pointer UM_Header_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s);
Pointer UM_Payload_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s);
Pointer UM_CPointer_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s);
Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset);
