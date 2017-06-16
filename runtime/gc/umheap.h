#if (defined (MLTON_GC_INTERNAL_TYPES))

typedef union GC_UM_Chunktype{

   struct GC_UM_Chunk* umChunk;
   struct GC_UM_Array_Chunk* umArrayChunk;

} GC_UM_Chunktype;

typedef struct UM_Mem_Chunk{
  // GC_UM_Chunktype chunkType;
   Word32_t chunkType;
   struct UM_Mem_Chunk* next_chunk;
} *UM_Mem_Chunk;


typedef struct GC_UM_Chunk {
    unsigned char ml_object[UM_CHUNK_PAYLOAD_SIZE + UM_CHUNK_PAYLOAD_SAFE_REGION];
    //    unsigned char ml_safe_region[UM_CHUNK_PAYLOAD_SAFE_REGION];
    Word32_t chunk_header;
    size_t sentinel;
    struct GC_UM_Chunk* next_chunk;
} *GC_UM_Chunk;

typedef struct GC_UM_heap {
    pointer start;
    pointer end;
    size_t size;
    UM_Mem_Chunk fl_head;
} *GC_UM_heap;

struct GC_UM_Array_Chunk;

typedef union GC_UM_Array_Payload {
    struct GC_UM_Array_Chunk* um_array_pointers[UM_CHUNK_ARRAY_INTERNAL_POINTERS];
    unsigned char ml_object[UM_CHUNK_ARRAY_PAYLOAD_SIZE];
} GC_UM_Array_Payload;

/* The array chunk. It still has a portion of ml header... for object type
   recognition. GC_arrayAllocate now returns p = &ml_array_payload, so that
   p - 4 returns the header of the array
*/
typedef struct GC_UM_Array_Chunk {
    Word32_t array_chunk_length;            /* Array Length                     */
    Word32_t array_chunk_ml_header;         /* MLton's array header             */
    GC_UM_Array_Payload ml_array_payload;   /* Payload or internal pointer      */
    Word32_t array_chunk_header;            /* For Mark / Sweep                 */
    Word32_t array_chunk_counter;           /* MLton's array counter            */
    Word32_t array_chunk_type;              /* Internal or Leaf                 */
    size_t array_height;                    /* Height of the tree (subtree)     */
    size_t array_chunk_numObjs;             /* Number of objects in leaf        */
    size_t array_num_chunks;                /* Number of leaf chunks            */
    size_t array_chunk_fan_out;             /* How many chunk in each ptr       */
    size_t array_chunk_objSize;             /* Object size of each element      */
    Word32_t array_chunk_magic;             /* A magic value for debug          */
    struct GC_UM_Array_Chunk* parent;       /* Pointer to parent chunk for iter */
    struct GC_UM_Array_Chunk* next_chunk;   /* For free list maintainance       */
    struct GC_UM_Array_Chunk* root;         /* For header to connect root       */
} *GC_UM_Array_Chunk;

/*typedef struct GC_UM_Array_heap {
    pointer start;
    pointer end;
    size_t size;
    UM_Mem_Chunk fl_head;
} *GC_UM_Array_heap;*/
#endif /* MLTON_GC_INTERNAL_TYPES */

#if (defined (MLTON_GC_INTERNAL_FUNCS))
static void insertFreeChunk(GC_state s, GC_UM_heap h, pointer c);
static void initUMHeap(GC_state s, GC_UM_heap h);
//static void initUMArrayHeap(GC_state s, GC_UM_heap h);
GC_UM_Array_Chunk insertArrayFreeChunk(GC_state s, GC_UM_heap h, pointer c);
GC_UM_Chunk allocNextChunk(GC_state s, GC_UM_heap h);
GC_UM_Array_Chunk allocNextArrayChunk(GC_state s, GC_UM_heap h);
bool createUMHeap(GC_state s, GC_UM_heap h,
                  size_t diredSize,
                  size_t minSize);
/*bool createUMArrayHeap(GC_state s, GC_UM_heap h,
                       size_t diredSize,
                       size_t minSize);*/
#endif
