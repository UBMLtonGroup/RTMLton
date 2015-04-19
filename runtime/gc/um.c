
/*
hello.1.c:(.text+0xb8fa): undefined reference to `UM_Header_alloc'
hello.1.c:(.text+0xb912): undefined reference to `UM_Payload_alloc'
hello.1.c:(.text+0xb92d): undefined reference to `UM_CPointer_offset'
*/

#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s(%x, %d, %d): %s\n", \
		__FILE__, __LINE__, __FUNCTION__, (x), (int)y, (int)z, m?m:"na")

/* define chunk structure (linked list)
 * define the free list
 */

/*
 * header
 * - 4 bytes MLton header (initialized in the ML code)
 * - 4 bytes next chunk pointer (initialize in the C code)
 */
Pointer
UM_Header_alloc(Pointer umfrontier, C_Size_t s)
{
    if (DEBUG_MEM)
        DBG(umfrontier, s, 0, "enter");
	return (umfrontier + s);
}

#define CHUNKSIZE 100

Pointer
UM_Payload_alloc(Pointer umfrontier, C_Size_t s)
{
	if (s > CHUNKSIZE) {
		// check free list
		// move frontier if nec'y
		// link chunks
		//
	}

	if (DEBUG_MEM)
		DBG(umfrontier, s, 0, "enter");
	return (umfrontier + s);
}


/*
 * calculate which chunk we need to look at
 *
 */
Pointer
UM_CPointer_offset(Pointer p, C_Size_t o, C_Size_t s)
{
	if (DEBUG_MEM)
       DBG(p, o, s, "enter");
	return (p + o);
}
