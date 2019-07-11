
/*
hello.1.c:(.text+0xb8fa): undefined reference to `UM_Header_alloc'
hello.1.c:(.text+0xb912): undefined reference to `UM_Payload_alloc'
hello.1.c:(.text+0xb92d): undefined reference to `UM_CPointer_offset'
*/


#undef DBG
#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s("FMTPTR", %d, %d): %s\n", \
		__FILE__, __LINE__, __func__, (uintptr_t)(x), (int)y, (int)z, m?m:"na")

/* define chunk structure (linked list)
 * define the free list
 */

/*
 * header
 * - 4 bytes MLton header (initialized in the ML code)
 * - 4 bytes next chunk pointer (initialize in the C code)
 */
Pointer
UM_Header_alloc(GC_state gc_stat,
                Pointer umfrontier,
                C_Size_t s)
{
    if (DEBUG_MEM)
        DBG(umfrontier, s, 0, "enter");

    return (umfrontier + s);
}

Pointer
UM_Object_alloc(GC_state gc_stat, C_Size_t num_chunks, uint32_t header, C_Size_t s)
{
    GC_UM_Chunk chunk = allocateChunks(gc_stat, &(gc_stat->umheap),num_chunks);
   
    assert(header !=0);

    uint32_t *alias = (uint32_t *) (chunk->ml_object) ;
    *alias = header;

    //violates C aliasing rules (undefined behavior)
    //*((uint32_t*) chunk->ml_object) = header;
    
    /*training wheels. Remove once confident. Use assert loop to verify other properties that must hold*/
    int i;
    GC_UM_Chunk current = chunk;
    for(i=0;i< num_chunks;i++)
    {
        assert(current->chunk_header & UM_CHUNK_IN_USE);
        current = current->next_chunk;
    }
   
    return (Pointer)(chunk->ml_object + s);
}




Pointer
UM_Payload_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s)
{
    fprintf(stderr,"In UM_PAYLOAD_ALLOC\n");
    if (DEBUG_MEM)
       DBG(umfrontier, s, 0, "enter");
    //    GC_collect(gc_stat, 0, false);
    //    GC_collect(gc_stat, 0, false);
    GC_UM_Chunk next_chunk = allocateChunks(gc_stat, &(gc_stat->umheap),1);
    GC_UM_Chunk current_chunk = (GC_UM_Chunk) umfrontier;
    current_chunk->chunk_header |= UM_CHUNK_IN_USE;

    if (DEBUG_MEM) {
        fprintf(stderr, "Sentinel: %d \n", current_chunk->sentinel);
        fprintf(stderr, "Nextchunk: "FMTPTR" \n", (uintptr_t) next_chunk);
    }
    current_chunk->next_chunk = NULL;

    if (s <= UM_CHUNK_PAYLOAD_SIZE) {
        if (DEBUG_MEM)
            DBG(umfrontier, s, 0, "move frontier to next chunk");
        return (Pointer)next_chunk->ml_object;
    }

    if (DEBUG_MEM)
       DBG(umfrontier, s, 0, "allocate next chunk");

    if (s > 2 * UM_CHUNK_PAYLOAD_SIZE) {
        die("BUG: Requiring allocation of more than 2 chunks\n");
    }
    /* Assume a maximum of 2 chunks
     * It can actually be 3 (fragmented chunks of LARGE objects)
     * TODO: Set header to represent chunked object
     */
    current_chunk->next_chunk = next_chunk;
/////////////////
    next_chunk->chunk_header |= UM_CHUNK_IN_USE;
/////////////////
    GC_UM_Chunk next_chunk_next = allocateChunks(gc_stat, &(gc_stat->umheap),1);
    next_chunk->next_chunk = NULL;

    if (DEBUG_MEM) {
        fprintf(stderr, "Next chunk next: place frontier at "FMTPTR"\n",
                (uintptr_t) next_chunk_next->ml_object);
    }

    return (Pointer) next_chunk_next->ml_object;
}

Pointer
UM_Chunk_Next_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s)
{
    GC_UM_Chunk current_chunk = (GC_UM_Chunk) (p - 4);
    return (Pointer)(current_chunk->next_chunk->ml_object + (o + 4 + s) -
                     UM_CHUNK_PAYLOAD_SIZE);
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

    GC_UM_Chunk current_chunk = (GC_UM_Chunk) (p - 4);
    if (current_chunk->chunk_header & UM_CHUNK_HEADER_CLEAN)
        die("Visiting a chunk that is on free list!\n");

    /* On current chunk */
    /* TODO: currently 4 is hard-coded mlton's header size */
    if (o + s + 4 <= UM_CHUNK_PAYLOAD_SIZE) {
        if (DEBUG_MEM) {
            DBG(p, o, s, "current chunk");
            fprintf(stderr, " sentinel: %d, val: "FMTPTR"\n",
                    current_chunk->sentinel,
                    *(p + o));
        }
        return (p + o);
    }

    if (DEBUG_MEM)
       DBG(p, o, s, "go to next chunk");
    return (Pointer)(current_chunk->next_chunk->ml_object + (o + 4 + s) -
                     UM_CHUNK_PAYLOAD_SIZE);
}


void writeBarrier(GC_state s,Pointer dstbase, Pointer srcbase)
{



    bool srcMarked,dstMarked = false;
    bool isSrcOnUMHeap = isObjectOnUMHeap(s,srcbase);
    bool isDstOnUMHeap = isObjectOnUMHeap(s,dstbase);


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
            uint16_t bytesNonObjptrs=0;
            uint16_t numObjptrs =0;
            GC_objectTypeTag tag = ERROR_TAG;
            splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);
            
            markChunk(srcbase,tag,GREY_MODE,s,numObjptrs);
        }
    }

    if(DEBUG_WB)
    {
        if(isSrcOnUMHeap || isDstOnUMHeap)
            fprintf(stderr,"%d]In writebarrier, srcbase= "FMTPTR", dstbase= "FMTPTR" , is dst marked? %s, is src marked? %s \n",PTHREAD_NUM,(uintptr_t)srcbase,(uintptr_t)dstbase, (dstMarked)?"YES":"NO", (srcMarked)?"YES":"NO" );
    }
   



    /*    if(isPointerMarkedByMode(dstbase, MARK_MODE))
            fprintf(stderr,"Marked\n");
        else if(!isPointerMarkedByMode(dstbase, MARK_MODE))
            fprintf(stderr,"UnMarked\n");
        else
            fprintf(stderr,"Grey mode ?\n");
            */
}

Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset) {
    Pointer heap_end = (gc_stat->umheap).start + (gc_stat->umheap).size;

    if (base < gc_stat->umheap.start || base >= heap_end) {
        if (DEBUG_MEM) {
            fprintf(stderr, "UM_Array_offset: not current heap: "FMTPTR" offset: %d\n",
                    (uintptr_t)base,offset);
        }
        return base + index * elemSize + offset;
    }

    GC_UM_Array_Chunk fst_leaf = (GC_UM_Array_Chunk)
        (base - GC_HEADER_SIZE - GC_HEADER_SIZE);

    if (fst_leaf->array_num_chunks <= 1) {
        return ((Pointer)&(fst_leaf->ml_array_payload.ml_object[0])) + index * elemSize + offset;
    }

    GC_UM_Array_Chunk root = fst_leaf->root;

    if (DEBUG_MEM) {
        fprintf(stderr, "UM_Array_offset: "FMTPTR" root: "
                FMTPTR", index: %d size: %d offset %d, "
                " length: %d, chunk_num_objs: %d\n",
                (uintptr_t)(base - 8), (uintptr_t)root, index, elemSize, offset,
                fst_leaf->array_chunk_length,
                fst_leaf->array_chunk_numObjs);
    }

    size_t chunk_index = index / root->array_chunk_numObjs;
    GC_UM_Array_Chunk current = root;
    size_t i;

    if (DEBUG_MEM) {
        fprintf(stderr, " >> Start to fetch chunk index: %d\n", chunk_index);
    }

    while (true) {
        if (current->array_chunk_header & UM_CHUNK_HEADER_CLEAN)
            die("Visiting a chunk that is on free list!\n");

        i = chunk_index / current->array_chunk_fan_out;
        if (DEBUG_MEM) {
            fprintf(stderr, "  --> chunk_index: %d, current fan out: %d, "
                    "in chunk index: %d\n",
                    chunk_index,
                    current->array_chunk_fan_out,
                    i);
        }
        chunk_index = chunk_index % current->array_chunk_fan_out;
        current = current->ml_array_payload.um_array_pointers[i];
        if (current->array_chunk_type == UM_CHUNK_ARRAY_LEAF) {
            size_t chunk_offset = (index % root->array_chunk_numObjs) * elemSize + offset;
            Pointer res = ((Pointer)&(current->ml_array_payload.ml_object[0])) +
                chunk_offset;
            if (DEBUG_MEM) {
                fprintf(stderr,
                        " --> Chunk_addr: "FMTPTR", index: %d, chunk base: "FMTPTR", "
                        "offset: %d, addr "FMTPTR" word: %x, %d, "
                        " char: %c\n",
                        (uintptr_t)current,
                        index,
                        (uintptr_t)current->ml_array_payload.ml_object, chunk_offset, (uintptr_t)res,
                        *((Word32_t*)(res)),
                        *((Word32_t*)(res)),
                        *((char*)(res)));
            }
            return res;
        }
    }

    die("UM_Array_Offset: shouldn't be here!");
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
        while(!CompareExchange((int *)&(s->casLock),-1,PTHREAD_NUM))
        {
            //NOP
            
        }
    }
    else if(lockOrUnlock == 0)
    {
        /*Case when you want to unlock*/

        if(!CompareExchange((int*)&(s->casLock),PTHREAD_NUM,-1))
            return;

    }
}


/*bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements an atomic compare and exchange operation. This compares the contents of *ptr with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired into *ptr. If they are not equal, the operation is a read and the current contents of *ptr are written into *expected. weak is true for weak compare_exchange, which may fail spuriously, and false for the strong variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter. When in doubt, use the strong variation.
If desired is written into *ptr then true is returned and memory is affected according to the memory order specified by success_memorder. There are no restrictions on what memory order can be used here.

Otherwise, false is returned and memory is affected according to failure_memorder. This memory order cannot be __ATOMIC_RELEASE nor __ATOMIC_ACQ_REL. It also cannot be a stronger order than that specified by success_memorder.

 * */
bool CompareExchange(int *ptr, int expected, int desired)
{
    
    return  __atomic_compare_exchange_n (ptr, &expected,desired, false, __ATOMIC_CONSUME, __ATOMIC_CONSUME);
    
}
