#if (defined (MLTON_GC_INTERNAL_TYPES))

#define UM_CHUNK_PAYLOAD_SIZE            32
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS 32
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      128
#define UM_CHUNK_SENTINEL_UNUSED         9999
#define UM_CHUNK_HEADER_CLEAN            0
#define UM_CHUNK_HEADER_MASK             1
#define UM_CHUNK_IN_USE                  2
#define UM_CHUNK_ARRAY_INTERNAL          0
#define UM_CHUNK_ARRAY_LEAF              1

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
