


#if (defined (MLTON_GC_INTERNAL_TYPES))
#define UM_CHUNK_PAYLOAD_SIZE            302 /*Change in include/c-chunk.h too*/
#define UM_CHUNK_PAYLOAD_SAFE_REGION     16
#define UM_CHUNK_ARRAY_PAYLOAD_SIZE      300 /* this should be divisible by platform wordsize */
#define UM_CHUNK_ARRAY_INTERNAL_POINTERS (UM_CHUNK_ARRAY_PAYLOAD_SIZE/sizeof(void*))

_Static_assert (UM_CHUNK_ARRAY_PAYLOAD_SIZE%sizeof(void*)==0,
		"UM_CHUNK_ARRAY_PAYLOAD_SIZE is not evenly divisible by platform word size");

#define UM_CHUNK_SENTINEL                0x9999
#define UM_ARRAY_SENTINEL                0x9998
#define UM_STACK_SENTINEL                0x9997

/*
 * Chunk header splitup
 * bits         purpose
 * 00-03        Chunk in use/clean
 * 04-07        Mark bits
 * 08-31        Object counter
 */

#define UM_CHUNK_HEADER_CLEAN            ((UM_header)0x0)
#define UM_CHUNK_IN_USE                  ((UM_header)0x80000000)
#define UM_CHUNK_MARK_MASK               ((UM_header)0x01000000)
#define UM_CHUNK_GREY_MASK               ((UM_header)0x02000000)
#define UM_CHUNK_RED_MASK                ((UM_header)0x04000000)
#define UM_CHUNK_UNMARK_MASK             ((UM_header)0xF0FFFFFF)
#define UM_CHUNK_ARRAY_INTERNAL          0x111
#define UM_CHUNK_ARRAY_LEAF              0xEAF
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
