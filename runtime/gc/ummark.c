void umDfsMarkObjectsUnMark(GC_state s, objptr *opp) {
    umDfsMarkObjects(s, opp, UNMARK_MODE);
}

void umDfsMarkObjectsMark(GC_state s, objptr *opp) {
    umDfsMarkObjects(s, opp, MARK_MODE);
}

void getObjectType(GC_state s, objptr *opp) {
    pointer p = objptrToPointer(*opp, s->heap.start);
    GC_header* headerp = getHeaderp(p);
    GC_header header = *headerp;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    GC_objectTypeTag tag;
    splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);
    switch (tag) {
    case NORMAL_TAG:
        fprintf(stderr, "NORMAL!\n");
        if (p >= s->umheap.start &&
            p < s->umheap.start + s->umheap.size) {
            fprintf(stderr, "  ON UM HEAP!\n");
        } else {
            fprintf(stderr, "  NOT ON UM HEAP\n");
        }
        break;
    case WEAK_TAG:
        fprintf(stderr, "WEAK!\n");
        break;
    case ARRAY_TAG:
        fprintf(stderr, "ARRAY!\n");
        break;
    case STACK_TAG:
        fprintf(stderr, "STACK\n");
        break;
    default:
        die("getObjetctType: swith: Shouldn't be here!\n");
    }
}

void umDfsMarkObjects(GC_state s, objptr *opp, GC_markMode m) {
    pointer p = objptrToPointer(*opp, s->heap.start);
    GC_header* headerp = getHeaderp(p);
    GC_header header = *headerp;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    GC_objectTypeTag tag;
    splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

    if (DEBUG_MEM)
        getObjectType(s, opp);

    /* Using MLton's header to track if it's marked */
    if (isPointerMarkedByMode(p, m)) {
        fprintf(stderr, FMTPTR"marked by mark_mode: %d, RETURN\n",
                (uintptr_t)p,
                (m == MARK_MODE));
        return;
    }

    if (m == MARK_MODE) {
        fprintf(stderr, FMTPTR" mark b pheader: %x, header: %x\n",
                (uintptr_t)p, *(getHeaderp(p)), header);
        header = header | MARK_MASK;
        *headerp = header;
        fprintf(stderr, FMTPTR" mark a pheader: %x, header: %x\n",
                (uintptr_t)p, *(getHeaderp(p)), header);
    } else {
        fprintf(stderr, FMTPTR" unmark b pheader: %x, header: %x\n",
                (uintptr_t)p, *(getHeaderp(p)), header);
        header = header & ~MARK_MASK;
        (*headerp) = header;
        fprintf(stderr, FMTPTR" unmark a pheader: %x, header: %x\n",
                (uintptr_t)p, *(getHeaderp(p)), header);
    }

    if (tag == NORMAL_TAG) {
        if (p >= s->umheap.start &&
            p < (s->umheap.start + s->umheap.size)) {
            GC_UM_Chunk pchunk = (GC_UM_Chunk)(p - GC_NORMAL_HEADER_SIZE);
            if (m == MARK_MODE) {
                pchunk->chunk_header |= UM_CHUNK_HEADER_MASK;
            } else {
                pchunk->chunk_header &= ~UM_CHUNK_HEADER_MASK;
            }

            if (DEBUG_MEM) {
                fprintf(stderr, "umDfsMarkObjects: chunk: "FMTPTR", sentinel: %d,"
                        " mark_mode: %d\n", (uintptr_t)pchunk, pchunk->sentinel,
                        (m == MARK_MODE));
            }

            if (NULL != pchunk->next_chunk) {
                pchunk->next_chunk->chunk_header ^= UM_CHUNK_HEADER_MASK;
            }
        }
    }

    if (numObjptrs > 0) {
        if (m == MARK_MODE)
            foreachObjptrInObject(s, p, umDfsMarkObjectsMark, true);
        else
            foreachObjptrInObject(s, p, umDfsMarkObjectsUnMark, true);
    }
}
