#include "../gc.h"


#if GC_MODEL_OBJPTR_SIZE == 32
# define POW ipow
#else
# define POW pow
#endif

#define LOCKIT(X) LOCK_DEBUG("LOCKIT"); IFED(pthread_mutex_lock(&(X)))
#define UNLOCKIT(X) IFED(pthread_mutex_unlock(&(X))); LOCK_DEBUG("UNLOCKIT")


#undef DBG
#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s("FMTPTR", %d, %d): %s\n", \
        __FILE__, __LINE__, __func__, (uintptr_t)(x), (int)y, (int)z, m?m:"na")

/* define chunk structure (linked list)
 * define the free list
 */

void GC_reserveAllocation(GC_state s, size_t numChunksToRequest) {
    reserveAllocation(s, numChunksToRequest);
}
  
void reserveAllocation(GC_state s, size_t numChunksToRequest) {
    assert (numChunksToRequest > 0);

    /* This what the gc-check pass inserts for all objects allocated in basic block.
     * Since the number of chunks for an array is calculated at runtime, it becomes necessary to do
     * the gc-check at runtime as well. This is different from the other allocations because
     * array allocation is treated as a CCall and therefore all temp registers are pushed to stack before
     * this function is called, thus allowing us to perform this check at runtime while preserving the
     * temporaries in the generated C code from being wrongfully collected.
     * I leave the heuristic check to the compiler inserted checkpoints
     */

    if(numChunksToRequest > s->maxChunksAvailable)
    {
        /* P.S. This check isn't "required" in compiler inserted reservation because blocks
         * don't allocate more than a few chunks. Runtime can possible allocate in thousands */
        die("Insufficient Memory: Reserving more space than available heap\n");
    }

    LOCK_FL;
    while ((s->fl_chunks < (s->reserved + numChunksToRequest))
            || (s->fl_chunks < (size_t)(s->hPercent * s->maxChunksAvailable))
          )
    {
        fprintf(stderr, "%d] "PURPLE("either fl (%d) is < reserved+req (%d) or fl (%d) < heuristic (%d)\n"),
            PTHREAD_NUM, s->fl_chunks, (s->reserved + numChunksToRequest), s->fl_chunks, (size_t)(s->hPercent * s->maxChunksAvailable) );
        UNLOCK_FL;
        pthread_yield();
        GC_collect(s, 0, true, true);
        LOCK_FL;
    }

    s->reserved += (numChunksToRequest==0?1:numChunksToRequest);
    UNLOCK_FL;

}

Pointer
UM_Object_alloc_no_packing(GC_state gc_stat, C_Size_t num_chunks, uint32_t header, C_Size_t s) {
    GC_UM_Chunk chunk;

    if (header == GC_STACK_HEADER) {
        chunk = allocateChunks(gc_stat, &(gc_stat->umheap), num_chunks, UM_STACK_CHUNK);
    } else {
        chunk = allocateChunks(gc_stat, &(gc_stat->umheap), num_chunks, UM_NORMAL_CHUNK);
    }

    // apply object header to all chunks in the chain
    GC_UM_Chunk current = chunk;
    for (int i = 0; i < num_chunks; i++) {
        //violates C aliasing rules (undefined behavior)
        //*((uint32_t*) chunk->ml_object) = header;

        uint32_t *alias = (uint32_t * )(current->ml_object);
        *alias = header;
        assert(current->chunk_header & UM_CHUNK_IN_USE);
        // default is UM_CHUNK_SENTINEL set in umheap::allocNextChunk
        if (header == GC_STACK_HEADER)
        	current->sentinel = UM_STACK_SENTINEL;
        current = current->next_chunk;
    }

    if (DEBUG_ALLOC_PACK)
        fprintf(stderr, "%d] %s returns %x\n", PTHREAD_NUM, __FUNCTION__, (unsigned int) (chunk->ml_object + s));
    return (Pointer) (chunk->ml_object + s);
}

Pointer
UM_Object_alloc_packing_stage2(GC_state s, C_Size_t num_chunks, uint32_t header, C_Size_t hdrsz, C_Size_t sz,
                                int *piggybackedOnThreadNum)
{
    /* stage2 packing: pack across threads
     * in this implementation, we allow threads in the same period to pack with each other.
     * an RMS will schedule these threads one after the other, starting at the beginning
     * of the period. the GC can run at the end of the period (but is not guaranteed to).
     *
     * we pack across threads by allowing the current thread that is requesting an alloc 
     * to look into other threads' activeChunks before allocating a new chunk of its own.
     *
     * stage1:
     * if no activeChunk or no space in activeChunk:
     *     peak into other threads' activeChunks <--- stage2 
     *     alloc a new activeChunk
     *
     */
    GC_UM_Chunk chunk = NULL;

    User_instrument_counter(320, num_chunks); /* JEFF packed2 total */

    /* see if any other thread's chunks have room */
    for (int i = 0 ; i < MAXPRI ; i++) {
        if (i == 1 || i == PTHREAD_NUM)
            continue;
        if (s->activeChunk[i] == BOGUS_POINTER)
            continue;

        LOCKIT(s->activeChunkLock[i]);

        if (((GC_UM_Chunk)s->activeChunk[i])->used + sz + hdrsz <= UM_CHUNK_PAYLOAD_SIZE){
            chunk = (GC_UM_Chunk)s->activeChunk[i];
            int used = chunk->used;
            header |= ((used+hdrsz)<<CHUNKOFFSET_SHIFT);
            chunk->used += (sz + hdrsz);

            UNLOCKIT(s->activeChunkLock[i]);

            LOCK_FL;
            s->reserved -= 1;
            UNLOCK_FL;

            if (DEBUG_ALLOC_PACK)
                fprintf(stderr, "%d]   writing header 0x%x(%d) with offset %d\n", 
                        PTHREAD_NUM, header, header, used+hdrsz);

            *piggybackedOnThreadNum = i;

            uint32_t *alias = (uint32_t *)(chunk->ml_object + used);
            *alias = header;
            return (Pointer) (chunk->ml_object + hdrsz + used);
        }

        UNLOCKIT(s->activeChunkLock[i]);
    }

    User_instrument_counter(340, num_chunks); /* JEFF packed2 cant fit, need new chunk */

    return BOGUS_POINTER;
}

Pointer
UM_Object_alloc_packing_stage1(GC_state s, C_Size_t num_chunks, uint32_t header, C_Size_t hdrsz, C_Size_t sz)
{
    GC_UM_Chunk chunk = NULL;
 
    // packing stage 1 (same thread can pack with its own regions)

    /* array allocs do not occur in this function. stack, thread, weak allocs are never packed. */

    assert (header != GC_THREAD_HEADER);
    assert (header != GC_STACK_HEADER);
    assert (header != GC_WEAK_GONE_HEADER);

    /* not a stack alloc. if we have an active chunk, pack into it.
     * if we do not have room, we will alloc a new chunk
     */
    assert (num_chunks == 1);

    if (s->activeChunk[PTHREAD_NUM] != BOGUS_POINTER) {
        chunk = (GC_UM_Chunk)s->activeChunk[PTHREAD_NUM];

        if (DEBUG_ALLOC_PACK)
            fprintf(stderr, "%d]   there's an activeChunk. used %d\n", PTHREAD_NUM, (int) chunk->used);

        /* if cur chunk has no room left, then alloc a new one:
         * if stage2 is enabled, we will first attempt to piggy back on another
         * thread's chunk.
         */

        if (chunk->used + sz + hdrsz > UM_CHUNK_PAYLOAD_SIZE) {

            if (s->packingStage2Enabled == true) {
                if (DEBUG_ALLOC_PACK)
                    fprintf(stderr, "%d]   no space in active chunk, cross-peak first\n", PTHREAD_NUM);
                int piggybackthread = -1;
                Pointer xp = UM_Object_alloc_packing_stage2(s, num_chunks, header, hdrsz, sz, &piggybackthread);
                if (xp != BOGUS_POINTER) {
                    if (DEBUG_ALLOC_PACK)
                        fprintf(stderr, "%d]   "PURPLE("successfully found space in another threads (%d) chunk\n"), 
                                PTHREAD_NUM,
                                piggybackthread
                        );
                    return xp;
                } else {
                    if (DEBUG_ALLOC_PACK)
                        fprintf(stderr, "%d]   unable to find space in another threads chunk, and:\n", PTHREAD_NUM);
                }
            } else {
                User_instrument_counter(330, num_chunks); /* JEFF packed1, new chunk required */
            }

            if (DEBUG_ALLOC_PACK)
                fprintf(stderr, "%d]   no space in active chunk, so allocating a new one\n", PTHREAD_NUM);

            chunk = allocateChunks(s, &(s->umheap), num_chunks, UM_NORMAL_CHUNK);
            s->activeChunk[PTHREAD_NUM] = (Pointer)chunk;
        }
        else {
            if (DEBUG_ALLOC_PACK)
                fprintf(stderr, "%d]   there is space in this chunk (%u) to fit our object (%d)\n",
                        PTHREAD_NUM,
                        (unsigned int)(UM_CHUNK_PAYLOAD_SIZE-(chunk->used)), sz+hdrsz
                );

            User_instrument_counter(310, num_chunks); /* JEFF packed1 no new chunk needed */

            LOCK_FL;
          	s->reserved -= 1;
            UNLOCK_FL;
        }
    }
    else {
        /* if stage2 is enabled, we will first attempt to piggy back on
         * another thread's chunk.
         */
        if (s->packingStage2Enabled == true) {
            if (DEBUG_ALLOC_PACK)
                fprintf(stderr, "%d]   no activeChunk, cross-peak first\n", PTHREAD_NUM);
            int piggybackthread = -1;
            Pointer xp = UM_Object_alloc_packing_stage2(s, num_chunks, header, hdrsz, sz, &piggybackthread);
            if (xp != BOGUS_POINTER) {
                if (DEBUG_ALLOC_PACK)
                    fprintf(stderr, "%d]   "PURPLE("successfully found space in another threads (%d) chunk\n"), PTHREAD_NUM,
                            piggybackthread
                    );
                return xp;
            } else {
                if (DEBUG_ALLOC_PACK)
                    fprintf(stderr, "%d]   unable to find space in another threads chunk, and:\n", PTHREAD_NUM);
                User_instrument_counter(330, num_chunks); /* JEFF packed1, new chunk required */
            }
        } else {
            User_instrument_counter(330, num_chunks); /* JEFF packed1, new chunk required */
        }

        if (DEBUG_ALLOC_PACK)
            fprintf(stderr, "%d]   no activeChunk, so allocating one.\n", PTHREAD_NUM);

        chunk = allocateChunks(s, &(s->umheap), num_chunks, UM_NORMAL_CHUNK);
        s->activeChunk[PTHREAD_NUM] = (Pointer)chunk;
    }

    /*
    the first object is at the start of the chunk, but is offset by
    's'. we want the header offset field to contain 0. so we do 

    header = *(objptr - s);
    offset = extract from header;
    start of chunk = objptr - offset; (note offset includes s)

    if another object is appended, we want the header offset to contain
    N, where N is the accumulation of previous (s+sz) objects as well
    as the current s, but not the current sz

    so if we use the chunk->used field to indicate how much of the
    chunk is in use, it should contain the sum of all of the previous
    (sz+s) values, but not the current s or sz. we will add the current
    s to used, and store that into the header offset. we then add our 
    (s+sz) to chunk->used, and then return chunk->used-sz so we mimic the
    behavior of returning the memory address just after the object 
    header field.
    */
    int used = chunk->used;
    header |= ((used+hdrsz)<<CHUNKOFFSET_SHIFT);
    chunk->used += (sz + hdrsz);

    if (DEBUG_ALLOC_PACK)
        fprintf(stderr, "%d]   writing header 0x%x(%d) with offset %d\n", 
                PTHREAD_NUM, header, header, used+hdrsz);

    // the following line violates C aliasing rules (undefined behavior):
    // *((uint32_t*) chunk->ml_object) = header;

    uint32_t *alias = (uint32_t *)(chunk->ml_object + used);
    *alias = header;
    return (Pointer) (chunk->ml_object + hdrsz + used);
}


Pointer
UM_Object_alloc(GC_state gc_stat, C_Size_t num_chunks, uint32_t header, C_Size_t s, C_Size_t sz) 
{
    /* gc_stat: GC_State 
       num_chunks: number of chunks requested
       header: the MLton object header
       s: the size of the MLton object header
       sz: the size of the object being placed into this chunk
           this field only applies to 'normal' (not stacks) objects
        
        This does:
        
        no-packing if gcstate.packingStage1Enabled is false. In this mode,
        each mlton object is mapped to an individual chunk. This is how RTMLton behaved
        in our papers upto 2021. this is also the default behavior if these runtime
        options are not specified.

        per-thread packing if gcstate.packingStage1Enabled is true and packingStage1Enabled is false.
        In this case, once a chunk is allocated, we save a ref to it (gcstate.activechunk[tnum])
        and keep track of how much of it is used. we continue to write new object headers into it
        until it is out of space, at which point we allocate a new chunk. since objptrs will be
        stashed on the stack, and stack walking will populate the worklist, we need to save which
        chunk we are in so the stack walking worklist can contain only chunks and not individual
        object pointers. we do this by saving the offset to the chunk header in the object header
        fields. see object.h for details.

        cross-thread packing if gcstate.packingStage1Enabled is true and packingStage1Enabled is true.
        In this case, each thread passes in a period counter (id) which is calculated from the
        current clock div the hyperperiod of the tasks. threads are allowed to place objects into
        other threads' chunks (activechunk) before allocating a new chunk for themselves.
     */
    Pointer p;
    GC_objectTypeTag tagRet;
    unsigned int objectTypeIndex = (header & TYPE_INDEX_MASK) >> TYPE_INDEX_SHIFT;
    splitHeader(gc_stat, header, &tagRet, NULL, NULL, NULL);

int lower_bound = 40; // JEFF
int upper_bound = 98;
char *lower_ev = getenv("LOWER");
char *upper_ev = getenv("UPPER");
if (lower_ev) lower_bound = atoi(lower_ev);
if (upper_ev) upper_bound = atoi(upper_ev);

    assert (num_chunks > 0);
    assert (header != 0);

    User_instrument_counter(50, num_chunks); /* JEFF um_obj_alloc calls, num chunks asked for */

    if (1||DEBUG_ALLOC) {
        fprintf(stderr, "%d]   UM_Object_alloc %s numchk:%u hd:0x%x(%d) (%s [%d] index:%u) sz:%d freelist:%d reserved:%d\n", 
                PTHREAD_NUM, 
                (header == GC_THREAD_HEADER)?"*** thread ***":"",
                num_chunks, header,
                header, objectTypeTagToString(tagRet), tagRet, objectTypeIndex, sz, 
                gc_stat->fl_chunks,
                gc_stat->reserved);
    }

    // no-packing. in order for stage2 to occur, stage1 must be enabled

    if (gc_stat->packingStage1Enabled == false || allowedToPack[PTHREAD_NUM] == 0) {
        if (header == GC_STACK_HEADER) {
            User_instrument_counter(100, 1); /* JEFF Stack Allocs */
            User_instrument_counter(110, num_chunks); /* JEFF Stack Allocs */
        }
        else
            User_instrument_counter(300, num_chunks); /* JEFF Normal allocs, unpacked */
        return UM_Object_alloc_no_packing(gc_stat, num_chunks, header, s);
    }

    /* packing enabled, but we only pack some objects types */

    if (header < lower_bound || header > upper_bound) { 
        if (header != GC_WORD8_VECTOR_HEADER || header == GC_THREAD_HEADER 
            || header == GC_STACK_HEADER 
            || header == GC_WEAK_GONE_HEADER) {
                if (DEBUG_ALLOC)
                    fprintf(stderr, "**   me=%d THR %d WEAK %d STACK %d\n", header,
                        GC_THREAD_HEADER, GC_WEAK_GONE_HEADER, GC_STACK_HEADER
                    );

                if (header == GC_STACK_HEADER) {
                    User_instrument_counter(100, 1); /* JEFF Stack Allocs */
                    User_instrument_counter(110, num_chunks); /* JEFF stack alloc */
                }
                else
                    User_instrument_counter(300, num_chunks); /* JEFF normal alloc, unpacked */

                if (DEBUG_ALLOC)
                    fprintf(stderr, RED("**   no packing for this type\n"));
                p = UM_Object_alloc_no_packing(gc_stat, num_chunks, header, s);
                if (DEBUG_ALLOC)
                    fprintf(stderr, "   UM_Object_alloc returns: p = %x\n", (unsigned int)p);
                return p;
        }
    }

    if (DEBUG_ALLOC_PACK)
        fprintf(stderr, YELLOW("**   going to pack %d\n"), header);

    // stage2 packing (if enabled) occurs as a part of stage1. 

    p = UM_Object_alloc_packing_stage1(gc_stat, num_chunks, header, s, sz);
    if (DEBUG_ALLOC_PACK)
        fprintf(stderr, "   UM_Object_alloc returns: p = %x\n", (unsigned int)p);
    return p;
}

Pointer
UM_Chunk_Next_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s)
{
    GC_UM_Chunk current_chunk = (GC_UM_Chunk) (p - sizeof(void *));
    return (Pointer)(current_chunk->next_chunk->ml_object + (o + sizeof(void *) + s) -
                     UM_CHUNK_PAYLOAD_SIZE);
}

bool
is_on_um_heap(GC_state s, Pointer p)
{
    /* Not on our heap! */
    if (p < (s->umheap).start || p >= (s->umheap).end) {
        return false;
    }
    return true;
}
/*
 * calculate which chunk we need to look at
 *
 */
Pointer
UM_CPointer_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s)
{
    if (DEBUG_MEM)
       DBG(p, o, s, "enter");

    Pointer heap_end = (gc_stat->umheap).end;

    /* Not on our heap! */
    if (p < (gc_stat->umheap).start ||
        p >= heap_end) {
        if (DEBUG_MEM)
            DBG(p, o, s, "not UM Heap");
        return (p + o);
    }

    GC_UM_Chunk current_chunk = (GC_UM_Chunk) (p - sizeof(void *));
    if (current_chunk->chunk_header & UM_CHUNK_HEADER_CLEAN)
        die("Visiting a chunk that is on free list!\n");

    /* On current chunk */
    /* TODO: currently 4 is hard-coded mlton's header size */
    if (o + s + 4 <= UM_CHUNK_PAYLOAD_SIZE) {
        if (DEBUG_MEM) {
            DBG(p, o, s, "current chunk");
            fprintf(stderr, " sentinel: %x, val: "FMTPTR"\n",
                    (int)current_chunk->sentinel,
                    (uintptr_t)*(p + o));
        }
        return (p + o);
    }

    if (DEBUG_MEM)
       DBG(p, o, s, "go to next chunk");
    return (Pointer)(current_chunk->next_chunk->ml_object + (o + sizeof(void *) + s) -
                     UM_CHUNK_PAYLOAD_SIZE);
}


void writeBarrier(GC_state s,Pointer dstbase, Pointer srcbase)
{
    bool srcMarked,dstMarked = false;
    bool isSrcOnUMHeap = isObjectOnUMHeap(s,srcbase);
    bool isDstOnUMHeap = isObjectOnUMHeap(s,dstbase);

    /*deadbeef dst indicates writing to stack slot.
     * in such case shade the source always, given that source is an objptr on umheap
     * e.g S(Objptr,16) = P_1[PTHREAD_NUM] */
    if(dstbase == (void *) 0xdeadbeef)
    {

        if(isSrcOnUMHeap)
        {
           srcMarked = (srcbase == NULL)?false:(isContainerChunkMarkedByMode(srcbase,MARK_MODE,getObjectType(s,srcbase)));

          if(!srcMarked)
          {
            GC_header header = getHeader(srcbase);
            uint16_t bytesNonObjptrs=0;
            uint16_t numObjptrs =0;
            GC_objectTypeTag tag = ERROR_TAG;
            splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

            markChunk(srcbase,tag,GREY_MODE,s,numObjptrs);
          }
        }

        return;

    }

    if(isDstOnUMHeap)
        dstMarked = (dstbase == NULL)?false:(isContainerChunkMarkedByMode(dstbase,MARK_MODE,getObjectType(s,dstbase)));

    if(isSrcOnUMHeap)
        srcMarked = (srcbase == NULL)?false:(isContainerChunkMarkedByMode(srcbase,MARK_MODE,getObjectType(s,srcbase)));

    if(dstMarked)
    {
        if(isSrcOnUMHeap && !srcMarked)
        {
           //objptr opp = pointerToObjptr((pointer)srcbase,s->heap.start);
           //die("We got one! \n");
            GC_header header = getHeader(srcbase);
            uint16_t bytesNonObjptrs= 0;
            uint16_t numObjptrs = 0;
            GC_objectTypeTag tag = ERROR_TAG;
            splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

            markChunk(srcbase,tag,GREY_MODE,s,numObjptrs);
        }
    }

    if (DEBUG_WB)
    {
        if(isSrcOnUMHeap || isDstOnUMHeap)
            fprintf(stderr,"%d] In writebarrier, srcbase= "FMTPTR", dstbase= "FMTPTR" , is dst marked? %s, is src marked? %s \n",
                PTHREAD_NUM,
                (uintptr_t)srcbase,
                (uintptr_t)dstbase,
                (dstMarked)?"YES":"NO",
                (srcMarked)?"YES":"NO");
    }
}

Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset) {
    Pointer heap_end = (gc_stat->umheap).start + (gc_stat->umheap).size;

    if (DEBUG_ARRAY_OFFSET)
        fprintf(stderr, "%d] UM_Array_offset(base="FMTPTR", index=%d, elemSize=%d, offset=%d)\n",
                PTHREAD_NUM, (uintptr_t)base, index, elemSize, offset);

    if (base < gc_stat->umheap.start || base >= heap_end) {
        if (DEBUG_ARRAY_OFFSET) {
            fprintf(stderr, "%d] UM_Array_offset: not current heap: "FMTPTR" offset: %d\n",
                    PTHREAD_NUM, (uintptr_t)base, offset);
        }
        return base + index * elemSize + offset;
    }

    /* base points to the ml_array_payload field (see GC_array_allocate)
     * and so we must deduct the array_chunk_ml_header and array_chunk_length fields
     * as well as the header field
     */
    GC_UM_Array_Chunk root = (GC_UM_Array_Chunk)(base - GC_HEADER_SIZE - sizeof(Word32_t));

    assert (root->array_chunk_magic == UM_ARRAY_SENTINEL);

    if (root->array_chunk_type == UM_CHUNK_ARRAY_LEAF) {
        if (DEBUG_MEM)
            fprintf(stderr, "   root type is LEAF so return direct calculation of base+%d\n",
                    index * elemSize + offset);

        Pointer res = ((Pointer)&(root->ml_array_payload.ml_object[0])) +
                index * elemSize + offset;
        if (DEBUG_MEM)
            fprintf(stderr,
                    " --> Chunk_addr: "FMTPTR", index: %d, chunk base: "FMTPTR", "
                    "offset: %d, addr "FMTPTR" word: %x, %d, "
                    " char: %c\n",
                    (uintptr_t)root,
                    index,
                    (uintptr_t)root->ml_array_payload.ml_object, index * elemSize + offset,
                    (uintptr_t)res,
                    *((Word32_t*)(res)),
                    *((Word32_t*)(res)),
                    *((char*)(res)));
        Pointer p1 = ((Pointer)&(root->ml_array_payload.ml_object[0])) + index * elemSize + offset;
        return p1;
    }
    else {
        if (DEBUG_MEM)
            fprintf(stderr, YELLOW("   root type is INTERNAL")
                    " so return indirect calculation by following root\n");
    }

    /* if we are here, base is the root of a tree */

    assert (root->array_height > 0);
    assert (root->array_chunk_magic == UM_ARRAY_SENTINEL);
    if (root->array_chunk_type != UM_CHUNK_ARRAY_INTERNAL) {
        fprintf(stderr, "%d] root-not-internal UM_Array_offset(base="FMTPTR", index=%d, elemSize=%d, offset=%d)\n",
                PTHREAD_NUM, (uintptr_t)base, index, elemSize, offset);

    }
    assert (root->array_chunk_type == UM_CHUNK_ARRAY_INTERNAL);

    /* to find the chunk that the index is in:
     *
     * we know the total number of elements in the array
     *   numElements  root->array_chunk_length
     * if we take index/num_els_per_chunk * UM_CHUNK_ARRAY_INTERNAL_POINTERS
     * we get the next chunk to go to when searching for our element's chunk.
     *
     * when we are on the next level of the tree, we know what the minimum
     * element number (e0) is that can possibly be linked from that node and the maximum
     * reachable element (eX)
     *
     * to determine what chunk to proceed to next:
     * (index-e0)/(eX-e0) * UM_CHUNK_ARRAY_INTERNAL_POINTERS
     *
     * we then adjust e0 and eX so they correspond to the
     * array slice reachable from the new root.
     *
     * this is the general approach to walking the tree
     *
     * 1. find the 'width' of the array slice below us
     * 2. calculate the next node we need to go to to find the element
     * 3. repeat so long as chunk_type is INTERNAL
     */

	size_t curHeight = root->array_height;
    GC_UM_Array_Chunk current = root;
	size_t width_in_elements = POW(UM_CHUNK_ARRAY_INTERNAL_POINTERS, curHeight) * root->num_els_per_chunk;
	size_t e0 = 0;
	size_t eX = width_in_elements-1;
	size_t slice_width_in_elements = width_in_elements;

    if (DEBUG_ARRAY_OFFSET) {
        fprintf(stderr, " IP^CH * EPC = %d^%d (%d) * %d = %d (max width_in_elements)\n",
                (int)UM_CHUNK_ARRAY_INTERNAL_POINTERS, (int)curHeight,
                POW(UM_CHUNK_ARRAY_INTERNAL_POINTERS, curHeight),
                (int)root->num_els_per_chunk, (int)width_in_elements);
        fprintf(stderr, " root->num_els_per_chunk %d\n", (int)root->num_els_per_chunk);
        fprintf(stderr, " e0 %d eX %d (starting case)\n", (int)e0, (int)eX);
    }

    while (current->array_chunk_type != UM_CHUNK_ARRAY_LEAF) {
        if (DEBUG_ARRAY_OFFSET) {
			GC_UM_Array_Chunk down = pointerToArrayChunk(current->ml_array_payload.um_array_pointers[0]);
            fprintf(stderr, "  chunk "FMTPTR" type %d ? leaf=%d\n",
                    (uintptr_t)&(current->ml_array_payload.ml_object[0]),
                    (int)current->array_chunk_type, UM_CHUNK_ARRAY_LEAF);
            fprintf(stderr, "   down-chunk type %d ? leaf=%d\n",
                    (int)down->array_chunk_type, UM_CHUNK_ARRAY_LEAF);
            fprintf(stderr, "  curHeight %d elsPerChunk %d\n", (int)curHeight, (int)root->num_els_per_chunk);
        }

        assert (current->array_chunk_magic == UM_ARRAY_SENTINEL);

		float next = floor((index-e0)/(float)slice_width_in_elements *
						   (float)(UM_CHUNK_ARRAY_INTERNAL_POINTERS))
		;

		/* now that we know where we are going next, we can figure out the new e0, eX,
		 * and slice width
		 */

		size_t num_leaf_chunks_below_next = POW(UM_CHUNK_ARRAY_INTERNAL_POINTERS, curHeight-1);
		e0 += (slice_width_in_elements / UM_CHUNK_ARRAY_INTERNAL_POINTERS) * floor(next);

		slice_width_in_elements = num_leaf_chunks_below_next * root->num_els_per_chunk;
		eX = e0 + slice_width_in_elements - 1;

        if (DEBUG_ARRAY_OFFSET)
            fprintf(stderr, "  leafs_below_next %d total width %d slice width %d\n",
                    (int)num_leaf_chunks_below_next,
                    (int)width_in_elements,
                    (int)slice_width_in_elements);

        assert (index >= e0);


        if (DEBUG_ARRAY_OFFSET)
            fprintf(stderr, "  next: %f = (%d-%d)/%d * %d, h=%d e/c=%d, e0 %d eX %d\n",
                    next, index, (int)e0, (int)slice_width_in_elements, (int)UM_CHUNK_ARRAY_INTERNAL_POINTERS,
                    (int)curHeight, (int)root->num_els_per_chunk, (int)e0, (int)eX);

        assert (current->ml_array_payload.um_array_pointers[(int)next] != NULL);
        current = pointerToArrayChunk(current->ml_array_payload.um_array_pointers[(int)next]);
        curHeight--;
    }

    /* now that we found the chunk that the index is in, we must adjust the index
     * based on the first index available in this chunk. so if we are looking for index
     * 455 and this chunk contains indices 450-500, we need to extract index 5 from this
     * chunk.
     */
	if (DEBUG_ARRAY_OFFSET)
		fprintf(stderr, "   (%d-%d)*%d+%d = %d = (index-e0) * elemSize + offset\n",
				index, (int)e0,
				elemSize, offset, (int)(index-e0)*elemSize+offset);

	assert (e0 <= index);
    assert ((index-e0) * elemSize + offset <= UM_CHUNK_ARRAY_PAYLOAD_SIZE);
    assert (current->array_chunk_type == UM_CHUNK_ARRAY_LEAF);

    Pointer res = ((Pointer)&(current->ml_array_payload.ml_object[0])) +
            (index-e0) * elemSize + offset;
    if (DEBUG_ARRAY_OFFSET)
        fprintf(stderr,
                " --> Chunk_addr: "FMTPTR", index: %d, chunk base: "FMTPTR", "
                "offset: %d, addr "FMTPTR" word: %x, %d, "
                " char: %c\n",
            (uintptr_t)root,
            index,
            (uintptr_t)root->ml_array_payload.ml_object, (int)(index-e0) * elemSize + offset,
            (uintptr_t)res,
            *((Word32_t*)(res)),
            *((Word32_t*)(res)),
            *((char*)(res)));

    Pointer p2 = ((Pointer)&(current->ml_array_payload.ml_object[0])) + (index-e0) * elemSize + offset;
    return p2;
}

/*When Compare and swap is called with lockOrUnlock =1, 
 * We check to see if mem location caslock is -1: 
 *  - If it is, we set it to pthread_num
 *  - if it isn't , we spinwait till caslock can be set
 * When called with lockorUnlock = 0 
 * We check to see if mem location caslock is set to current PTHREAD_NUM
 *  - If it is, we set it to -1
 *  - If it isn't we just return cause obviously the calling thread didnt lock it
 */
void CompareAndSet(GC_state s, int lockOrUnlock)
{
    if(lockOrUnlock ==1)
    {
        /*Case when you want to lock*/
        while(!CompareExchange(&(s->casLock),-1,PTHREAD_NUM))
        {
            //NOP

        }
    }
    else if(lockOrUnlock == 0)
    {
        /*Case when you want to unlock*/

        if(!CompareExchange(&(s->casLock),PTHREAD_NUM,-1))
            return;

    }
}


/* bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)

   This built-in function implements an atomic compare and exchange operation. This compares the contents of *ptr
   with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired
   into *ptr. If they are not equal, the operation is a read and the current contents of *ptr are written
   into *expected. weak is true for weak compare_exchange, which may fail spuriously, and false for the strong
   variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter.
   When in doubt, use the strong variation.

   If desired is written into *ptr then true is returned and memory is affected according to the memory order
   specified by success_memorder. There are no restrictions on what memory order can be used here.

   Otherwise, false is returned and memory is affected according to failure_memorder. This memory order cannot
   be __ATOMIC_RELEASE nor __ATOMIC_ACQ_REL. It also cannot be a stronger order than that specified by success_memorder.

 */
bool CompareExchange(volatile int *ptr, int expected, int desired)
{
    return  __atomic_compare_exchange_n (ptr, &expected,desired, false, __ATOMIC_CONSUME, __ATOMIC_CONSUME);
}
