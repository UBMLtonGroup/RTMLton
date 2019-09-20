#if (defined (MLTON_GC_INTERNAL_TYPES))
#define UM_CHUNK_PAYLOAD_SIZE            154
#define UM_CHUNK_PAYLOAD_SAFE_REGION     16
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS 32
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      128
#define UM_CHUNK_SENTINEL_UNUSED         9999

/*
 * Chunk header splitup
 * bits         purpose
 * 00-03       Chunk in use/clean
 * 04-07    Mark bits
 * 08-31    Object counter
 */

#define UM_CHUNK_HEADER_CLEAN            ((UM_header)0x0)
#define UM_CHUNK_IN_USE                  ((UM_header)0x80000000)
#define UM_CHUNK_MARK_MASK               ((UM_header)0x01000000)
#define UM_CHUNK_GREY_MASK               ((UM_header)0x02000000)
#define UM_CHUNK_RED_MASK                ((UM_header)0x04000000)
#define UM_CHUNK_UNMARK_MASK             ((UM_header)0xF0FFFFFF)
#define UM_CHUNK_ARRAY_INTERNAL          0
#define UM_CHUNK_ARRAY_LEAF              1
#define UM_EMPTY                         0
#define UM_NORMAL_CHUNK                  1
#define UM_ARRAY_CHUNK                   2

#define ISINUSE(header) (header & UM_CHUNK_IN_USE)
#define ISMARKED(header) (header & UM_CHUNK_MARK_MASK)
#define ISGREY(header) (header & UM_CHUNK_GREY_MASK)
#define ISRED(header) (header & UM_CHUNK_RED_MASK)
#define ISUNMARKED(header) (~(header & ~UM_CHUNK_UNMARK_MASK))
#define UNMARK(header) do {header &= ~UM_CHUNK_MARK_MASK;} while(0)
#endif
