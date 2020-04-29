
/*
 * Currently GC_UM_CHUNK with UM_CHUNK_PAYLOAD_SIZE =154 is 208 bytes
 * To match Array chunk sizes to utilize max space:
 * with UM_CHUNK_ARRAY_PAYLOAD_SIZE 128, difference between the two is 
 * 24 bytes. 
 * Adding 24 bytes to array payload --> 128+24 = 152
 * This means internal array nodes can now hold 152/4 = 38 internal pointers.
 *  Size of GC_UM_CHUNK = size of GC_UM_ARRAY_CHUNK = 208
 *
 *  Maintain this whenever the structs in umheap.h are changed or when payload sizes are changed
 *
 * */


#if (defined (MLTON_GC_INTERNAL_TYPES))
#define UM_CHUNK_PAYLOAD_SIZE            302 /*Change in include/c-chunk.h too*/
#define UM_CHUNK_PAYLOAD_SAFE_REGION     16
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS 75
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      300

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
#define UM_STACK_CHUNK                   3

#define ISINUSE(header) (header & UM_CHUNK_IN_USE)
#define ISMARKED(header) (header & UM_CHUNK_MARK_MASK)
#define ISGREY(header) (header & UM_CHUNK_GREY_MASK)
#define ISRED(header) (header & UM_CHUNK_RED_MASK)
#define ISUNMARKED(header) (~(header & ~UM_CHUNK_UNMARK_MASK))
#define UNMARK(header) do {header &= ~UM_CHUNK_MARK_MASK;} while(0)
#endif
