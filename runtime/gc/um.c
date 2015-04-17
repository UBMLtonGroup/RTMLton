
/*
hello.1.c:(.text+0xb8fa): undefined reference to `UM_Header_alloc'
hello.1.c:(.text+0xb912): undefined reference to `UM_Payload_alloc'
hello.1.c:(.text+0xb92d): undefined reference to `UM_CPointer_offset'
*/

#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s(%x, %d, %d): %s\n", \
		__FILE__, __LINE__, __FUNCTION__, (x), (int)y, (int)z, m?m:"na")

Pointer
UM_Header_alloc(GC_state* gc_stat, Pointer umfrontier, C_Size_t s)
{
   if (DEBUG_MEM)
        DBG(umfrontier, s, 0, "enter");
	return (umfrontier + s);
}

Pointer
UM_Payload_alloc(GC_state* gc_stat, Pointer umfrontier, C_Size_t s)
{
	if (DEBUG_MEM)
       DBG(umfrontier, s, 0, "enter");
	return (umfrontier + s);
}

Pointer
UM_CPointer_offset(GC_state* gc_stat, Pointer p, C_Size_t o, C_Size_t s)
{
	if (DEBUG_MEM)
       DBG(p, o, s, "enter");
	return (p + o);
}
