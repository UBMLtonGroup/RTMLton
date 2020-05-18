#include "platform.h"
#define MLTON_GC_INTERNAL_TYPES
#include "../gc/um_constants.h"

pointer UM_walk_array_leafs(pointer _c, size_t *nels);

static void __attribute__ ((unused))
Stdio_print_real(String8_t s, FILE *fd) {
	uintmax_t size = GC_getArrayLength((pointer) s);

	size_t nels = 0, write_rv = 0;
	String8_t s2 = s;
	int whats_left = size, to_write = UM_CHUNK_ARRAY_PAYLOAD_SIZE;

	while(whats_left || (s2 = UM_walk_array_leafs(s2, &nels))) {
		to_write = min(UM_CHUNK_ARRAY_PAYLOAD_SIZE, whats_left);
		whats_left -= to_write;
		write_rv += fwrite ((const void *) s2, to_write, 1, fd);
	}
}

void Stdio_printStderr(String8_t s) {
	Stdio_print_real(s, stderr);
	//uintmax_t size = GC_getArrayLength((pointer) s);
	//fwrite((const void *) s, (size_t) size, 1, stderr);
}

void Stdio_printStdout(String8_t s) {
	Stdio_print_real(s, stdout);
	//uintmax_t size = GC_getArrayLength((pointer) s);
	//fwrite((const void *) s, (size_t) size, 1, stdout);
}


void Stdio_print(String8_t s) {
	Stdio_printStderr(s);
}

