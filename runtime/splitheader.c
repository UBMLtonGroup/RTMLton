#include <stdlib.h>
#undef _POSIX_C_SOURCE
#define MLTON_GC_INTERNAL_TYPES
#define MLTON_GC_INTERNAL_FUNCS
#define MLTON_GC_INTERNAL_BASIS
#include "platform.h"

GC_header buildHeaderFromTypeIndex (uint32_t t) {
  assert (t < TWOPOWER (TYPE_INDEX_BITS));
  return 1 | (t << 1);
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#include "gc.h"
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("splitheader <headervalue>\n");
        exit(0);
    }
    GC_header header = atoi(argv[1]);
    GC_objectTypeTag tagRet;
    bool hasIdentityRet;
    uint16_t bytesNonObjptrsRet;
    uint16_t numObjptrsRet;
    printf("GC_STACK_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_STACK_HEADER, 
           BYTE_TO_BINARY(GC_STACK_HEADER));
    printf("GC_THREAD_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_THREAD_HEADER,
           BYTE_TO_BINARY(GC_THREAD_HEADER));
    printf("GC_WEAK_GONE_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_WEAK_GONE_HEADER,
           BYTE_TO_BINARY(GC_WEAK_GONE_HEADER));
    printf("GC_WORD8_VECTOR_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_WORD8_VECTOR_HEADER,
           BYTE_TO_BINARY(GC_WORD8_VECTOR_HEADER));
    printf("GC_WORD16_VECTOR_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_WORD16_VECTOR_HEADER,
           BYTE_TO_BINARY(GC_WORD16_VECTOR_HEADER));
    printf("GC_WORD32_VECTOR_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_WORD32_VECTOR_HEADER,
           BYTE_TO_BINARY(GC_WORD32_VECTOR_HEADER));
    printf("GC_WORD64_VECTOR_HEADER %d "BYTE_TO_BINARY_PATTERN"\n", GC_WORD64_VECTOR_HEADER,
           BYTE_TO_BINARY(GC_WORD64_VECTOR_HEADER));

    printf("%d "BYTE_TO_BINARY_PATTERN"\n", header, BYTE_TO_BINARY(header));
    exit(0);
}

