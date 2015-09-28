#if (defined (MLTON_GC_INTERNAL_TYPES))
#define UM_CHUNK_PAYLOAD_SIZE            64
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS 32
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      128
#define UM_CHUNK_SENTINEL_UNUSED         9999
#define UM_CHUNK_HEADER_CLEAN            0
#define UM_CHUNK_HEADER_MASK             1
#define UM_CHUNK_IN_USE                  2
#define UM_CHUNK_ARRAY_INTERNAL          0
#define UM_CHUNK_ARRAY_LEAF              1
#endif
