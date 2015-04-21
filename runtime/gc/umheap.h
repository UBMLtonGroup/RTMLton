#if (defined (MLTON_GC_INTERNAL_TYPES))

#define UM_CHUNK_PAYLOAD_SIZE 32
#define UM_CHUNK_SENTINEL_UNUSED 9999

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

typedef struct GC_UM_Array_heap {
    pointer start;
    size_t size;
} *GC_UM_Array_heap;
#endif /* MLTON_GC_INTERNAL_TYPES */

#if (defined (MLTON_GC_INTERNAL_FUNCS))
static void insertFreeChunk(GC_state s, GC_UM_heap h, pointer c);
static void initUMHeap(GC_state s, GC_UM_heap h);
GC_UM_Chunk allocNextChunk(GC_state s, GC_UM_heap h);
bool createUMHeap(GC_state s, GC_UM_heap h,
                  size_t diredSize,
                  size_t minSize);
bool createUMArrayHeap(GC_state s, GC_UM_Array_heap h,
                       size_t diredSize,
                       size_t minSize);
#endif
