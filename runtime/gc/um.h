//#define PROFILE_UMGC
Pointer UM_Header_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s);
Pointer UM_Payload_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s);
Pointer UM_CPointer_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s);
Pointer UM_Chunk_Next_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s);
Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset);
Pointer UM_Object_alloc(GC_state gc_stat, C_Size_t num_chunks, uint32_t header,
                        C_Size_t s);


void CompareAndSet(GC_state s, int lockOrUnlock);
bool CompareExchange(int *ptr, int expected, int desired);


void writeBarrier(GC_state s,Pointer dstbase, Pointer srcbase);
