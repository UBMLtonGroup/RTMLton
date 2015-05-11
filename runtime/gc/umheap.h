#if (defined (MLTON_GC_INTERNAL_TYPES))

#define UM_CHUNK_PAYLOAD_SIZE            32
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS 32
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      128
#define UM_CHUNK_SENTINEL_UNUSED         9999
#define UM_CHUNK_HEADER_CLEAN            0
#define UM_CHUNK_HEADER_MASK             1
#define UM_CHUNK_IN_USE                  2
#define UM_CHUNK_ARRAY_INTERNAL          4
#define UM_CHUNK_ARRAY_LEAF              8

typedef struct GC_UM_Chunk {
    unsigned char ml_object[UM_CHUNK_PAYLOAD_SIZE];
    Word32_t chunk_header;
    size_t sentinel;
    struct GC_UM_Chunk* next_chunk;
} *GC_UM_Chunk;

typedef struct GC_UM_heap {
    pointer start;
    size_t size;
    int fl_chunks;
    GC_UM_Chunk fl_head;
} *GC_UM_heap;

struct GC_UM_Array_Chunk;

typedef union GC_UM_Array_Payload {
    struct GC_UM_Array_Chunk* um_array_pointers[UM_CHUNK_ARRAY_INTERNAL_POINTERS];
    unsigned char ml_object[UM_CHUNK_ARRAY_PAYLOAD_SIZE];
} GC_UM_Array_Payload;

typedef struct GC_UM_Array_Chunk {
    GC_UM_Array_Payload ml_array_payload;
    Word32_t array_chunk_counter;
    Word32_t array_chunk_length;
    Word32_t array_chunk_ml_header;
    Word32_t array_chunk_header;
    struct GC_UM_Array_Chunk* next_chunk;
} *GC_UM_Array_Chunk;

typedef struct GC_UM_Array_heap {
    pointer start;
    size_t size;
    size_t fl_array_chunks;
    GC_UM_Array_Chunk fl_array_head;
} *GC_UM_Array_heap;
#endif /* MLTON_GC_INTERNAL_TYPES */

#if (defined (MLTON_GC_INTERNAL_FUNCS))
static void insertFreeChunk(GC_state s, GC_UM_heap h, pointer c);
static void initUMHeap(GC_state s, GC_UM_heap h);
static void initUMArrayHeap(GC_state s, GC_UM_Array_heap h);
static void insertArrayFreeChunk(GC_state s, GC_UM_Array_heap h, pointer c);
GC_UM_Chunk allocNextChunk(GC_state s, GC_UM_heap h);
GC_UM_Array_Chunk allocNextArrayChunk(GC_state s, GC_UM_Array_heap h);
bool createUMHeap(GC_state s, GC_UM_heap h,
                  size_t diredSize,
                  size_t minSize);
bool createUMArrayHeap(GC_state s, GC_UM_Array_heap h,
                       size_t diredSize,
                       size_t minSize);
#endif
