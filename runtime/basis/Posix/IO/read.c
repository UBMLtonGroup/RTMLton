#include "platform.h"
#define MLTON_GC_INTERNAL_TYPES
#include "../gc/um_constants.h"

pointer UM_walk_array_leafs(pointer _c, size_t *nels);

static inline C_Errno_t(C_SSize_t)
Posix_IO_reada (C_Fd_t fd, Pointer b,
		        C_Int_t i, C_Size_t s) {
	/* read into the array of characters starting at index 'i'
	 * and going for 's' characters.
	 */
	size_t nels = 0, read_rv = 0, total_read = 0;
	String8_t s2 = b;
	bool first_time = true;
	int start = i, whats_left = s, to_read = UM_CHUNK_ARRAY_PAYLOAD_SIZE;
//printf("%s(%d, %x, %d, %d)\n", __FUNCTION__, fd, (unsigned int)b, i, s);
	while((s2 = UM_walk_array_leafs(s2, &nels))) {
		to_read = min(UM_CHUNK_ARRAY_PAYLOAD_SIZE - start, whats_left);
		whats_left -= to_read;
		if (whats_left <= 0) break;
		read_rv = read (fd, (void *) ((char *) s2 + start), to_read);
		//printf("   read:%d into %x\n", read_rv, (unsigned int)s2);
		if (read_rv == 0) break; // EOF
		total_read += read_rv;
		if (first_time) {
			start = 0;
			first_time = false;
		}
    }
	return total_read;
}

static inline C_Errno_t(C_SSize_t)
Posix_IO_read (C_Fd_t fd, Pointer b,
               C_Int_t i, C_Size_t s) {
	//printf("%s\n", __FUNCTION__);
	return Posix_IO_reada (fd, b, i, s);
//	return read (fd, (void *) ((char *) b + i), s);
}

C_Errno_t(C_SSize_t)
Posix_IO_readChar8 (C_Fd_t fd, Array(Char8) b,
                   C_Int_t i, C_Size_t s) {
	//printf("%s\n", __FUNCTION__);
	return Posix_IO_read (fd, (Pointer)b, i, s);
}

C_Errno_t(C_SSize_t)
Posix_IO_readWord8 (C_Fd_t fd, Array(Word8) b,
                    C_Int_t i, C_Size_t s) {
	//printf("%s\n", __FUNCTION__);
	return Posix_IO_read (fd, (Pointer)b, i, s);
}
