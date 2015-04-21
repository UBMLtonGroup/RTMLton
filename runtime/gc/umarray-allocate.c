pointer GC_arrayAllocate (GC_state s,
                          __attribute__ ((unused)) size_t ensureBytesFree,
                          GC_arrayLength numElements,
                          GC_header header) {
    size_t arraySize;
    size_t bytesPerElement;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    pointer frontier, last;
    pointer result;

    splitHeader(s, header, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
    bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);
    arraySize = bytesPerElement * numElements;
    arraySize += GC_ARRAY_HEADER_SIZE;

    frontier = s->umarfrontier;
    last = frontier + arraySize;
    s->umarfrontier = last;

    if (last >= s->umarheap.start + s->umarheap.size) {
        die("Array allocate out of memory!\n");
    }

    *((GC_arrayCounter*)(frontier)) = 0;
    frontier = frontier + GC_ARRAY_COUNTER_SIZE;
    *((GC_arrayLength*)(frontier)) = numElements;
    frontier = frontier + GC_ARRAY_LENGTH_SIZE;
    *((GC_header*)(frontier)) = header;
    frontier = frontier + GC_HEADER_SIZE;

    result = frontier;

    if (1 <= numObjptrs and 0 < numElements) {
        pointer p;

        if (0 == bytesNonObjptrs)
            for (p = frontier; p < last; p += OBJPTR_SIZE)
                *((objptr*)p) = BOGUS_OBJPTR;
        else {
            /* Array with a mix of pointers and non-pointers. */
            size_t bytesObjptrs;

            bytesObjptrs = numObjptrs * OBJPTR_SIZE;

            for (p = frontier; p < last; ) {
                pointer next;

                p += bytesNonObjptrs;
                next = p + bytesObjptrs;
                assert (next <= last);
                for ( ; p < next; p += OBJPTR_SIZE)
                    *((objptr*)p) = BOGUS_OBJPTR;
            }
        }
    }

    return result;
}
