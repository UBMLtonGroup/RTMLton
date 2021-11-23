#include "../gc.h"


#if GC_MODEL_OBJPTR_SIZE == 32
# define POW ipow
#else
# define POW pow
#endif



#undef DBG
#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s("FMTPTR", %d, %d): %s\n", \
        __FILE__, __LINE__, __func__, (uintptr_t)(x), (int)y, (int)z, m?m:"na")

/* define chunk structure (linked list)
 * define the free list
 */


void reserveAllocation(GC_state s, size_t numChunksToRequest) {
    assert (numChunksToRequest > 0);

    /* This what the gc-check pass inserts for all objects allocated in basic block.
     * SInce the number of chunks for an array is calculated at runtime, it becomes necessary to do
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
    while (s->fl_chunks < (s->reserved + numChunksToRequest))
    {
        UNLOCK_FL;
        GC_collect(s, 0, true, true);
        LOCK_FL;
    }


    s->reserved += (numChunksToRequest==0?1:numChunksToRequest);
    UNLOCK_FL;

}


Pointer
UM_Object_alloc(GC_state gc_stat, C_Size_t num_chunks, uint32_t header, C_Size_t s) {
    GC_UM_Chunk chunk;

    assert (num_chunks > 0);

    if (header == GC_STACK_HEADER) {
        chunk = allocateChunks(gc_stat, &(gc_stat->umheap), num_chunks, UM_STACK_CHUNK);
    } else {
        chunk = allocateChunks(gc_stat, &(gc_stat->umheap), num_chunks, UM_NORMAL_CHUNK);
    }

    assert(header != 0);

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

    return (Pointer) (chunk->ml_object + s);
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
            fprintf(stderr,"%d]In writebarrier, srcbase= "FMTPTR", dstbase= "FMTPTR" , is dst marked? %s, is src marked? %s \n",PTHREAD_NUM,(uintptr_t)srcbase,(uintptr_t)dstbase, (dstMarked)?"YES":"NO", (srcMarked)?"YES":"NO" );
    }
}

Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset) {
    Pointer heap_end = (gc_stat->umheap).start + (gc_stat->umheap).size;

    if (DEBUG_ARRAY_OFFSET)
        fprintf(stderr, "UM_Array_offset(base="FMTPTR", index=%d, elemSize=%d, offset=%d)\n",
                (uintptr_t)base, index, elemSize, offset);

    if (base < gc_stat->umheap.start || base >= heap_end) {
        if (DEBUG_ARRAY_OFFSET) {
            fprintf(stderr, "UM_Array_offset: not current heap: "FMTPTR" offset: %d\n",
                    (uintptr_t)base, offset);
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
