
/*
hello.1.c:(.text+0xb8fa): undefined reference to `UM_Header_alloc'
hello.1.c:(.text+0xb912): undefined reference to `UM_Payload_alloc'
hello.1.c:(.text+0xb92d): undefined reference to `UM_CPointer_offset'
*/

#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s(%016llx, %d, %d): %s\n", \
		__FILE__, __LINE__, __FUNCTION__, (long long unsigned int)x, (int)y, (int)z, m?m:"na")

Pointer
UM_Header_alloc(Pointer umfrontier, C_Size_t s)
{
	DBG(umfrontier, s, 0, "enter");
	return umfrontier+s;
}

Pointer
UM_Payload_alloc(Pointer umfrontier, C_Size_t s)
{
	DBG(umfrontier, s, 0, "enter");
	umfrontier += s;
	return umfrontier;
}

Pointer
UM_CPointer_offset(Pointer p, C_Size_t o, C_Size_t s)
{
	DBG(p, o, s, "enter");
	return p+o;
}

