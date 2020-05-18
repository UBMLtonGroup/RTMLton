#include "platform.h"
#define MLTON_GC_INTERNAL_TYPES
#include "../gc/um_constants.h"

pointer UM_walk_array_leafs(pointer _c, size_t *nels);

static inline C_Errno_t(C_SSize_t)
Posix_IO_writea (C_Fd_t fd, Pointer b,
                 C_Int_t i, C_Size_t s) {
	/* write out the array of characters starting at index 'i'
	 * and going for 's' characters.
	 */
	size_t nels = 0, write_rv = 0, write_total = 0;
	String8_t s2 = b;
	bool first_time = true;
	int start = i, whats_left = s, to_write = UM_CHUNK_ARRAY_PAYLOAD_SIZE;
    //printf("%s(%d, %x, %d, %d)\n", __FUNCTION__, fd, (unsigned int)b, i, s);

	while((s2 = UM_walk_array_leafs(s2, &nels))) {
		to_write = min(UM_CHUNK_ARRAY_PAYLOAD_SIZE - start, whats_left);
		whats_left -= to_write;
		write_rv = write (fd, (void *) ((char *) s2 + start), to_write);
		if (write_rv < 0) return write_rv;
		write_total += write_rv;
		if (first_time) {
			start = 0;
			first_time = false;
		}
		if (whats_left <= 0) break;
	}
	return write_total;
}

static inline C_Errno_t(C_SSize_t)
Posix_IO_writev (C_Fd_t fd, Pointer b,
		         C_Int_t i, C_Size_t s) {
    //printf("%s(%d, %x, %d, %d)\n", __FUNCTION__, fd, (unsigned int)b, i, s);
    return write (fd, (void *) ((char *) b + i), s);
}

C_Errno_t(C_SSize_t)
Posix_IO_writeChar8Arr (C_Fd_t fd, Array(Char8_t) b,
                        C_Int_t i, C_Size_t s) {
	return Posix_IO_writea (fd, (Pointer)b, i, s);
}

C_Errno_t(C_SSize_t)
Posix_IO_writeChar8Vec (C_Fd_t fd, Vector(Char8_t) b,
                        C_Int_t i, C_Size_t s) {
	return Posix_IO_writev (fd, (Pointer)b, i, s);
}

C_Errno_t(C_SSize_t)
Posix_IO_writeWord8Arr (C_Fd_t fd, Array(Word8_t) b,
                        C_Int_t i, C_Size_t s) {
	return Posix_IO_writea (fd, (Pointer)b, i, s);
}

C_Errno_t(C_SSize_t)
Posix_IO_writeWord8Vec (C_Fd_t fd, Vector(Word8_t) b,
                        C_Int_t i, C_Size_t s) {
	return Posix_IO_writev (fd, (Pointer)b, i, s);
}
