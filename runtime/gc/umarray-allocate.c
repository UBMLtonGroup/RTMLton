#include "../gc.h"

static int ipow(int, unsigned int);

static /* see doc/rtmlton/pow-or-ipow.md */
int ipow(int base, unsigned int exp)
{
	if (exp == 0) return 1;

	int result = 1;
	for (;;)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		if (!exp)
			break;
		base *= base;
	}

	return result;
}

static int getLengthOfList(GC_UM_Array_Chunk head);

static
int getLengthOfList(GC_UM_Array_Chunk head) {
	GC_UM_Array_Chunk current = head;
	int i = 0;
	while (current != NULL) {
		current = current->next_chunk;
		i++;
	}

	return i;
}

static
GC_UM_Array_Chunk create_array_tree(GC_UM_Array_Chunk root,
									GC_UM_Array_Chunk *chunks,
									size_t numChunks,
						 			size_t height,
									GC_header header,
									size_t numElements,
									GC_UM_Array_Chunk rootroot);


#define POPCHUNK(allocHead, chunk)             \
   do { GC_UM_Array_Chunk new = allocHead;     \
        assert(new != NULL);                   \
        allocHead = (allocHead)->next_chunk;   \
        new->next_chunk = NULL;                \
        assert (new->array_chunk_magic == UM_ARRAY_SENTINEL); \
        chunk = new;                           \
      } while(0)

__attribute__((unused)) static
GC_UM_Array_Chunk create_array_tree(GC_UM_Array_Chunk root,
									GC_UM_Array_Chunk *chunks,
									size_t numChunks,
									size_t height,
									GC_header header,
									size_t numElements,
									GC_UM_Array_Chunk rootroot) {

	/* base case, H=0, initialize the chunks as leaves and return
	 */
	if (height == 0) {
		assert (getLengthOfList(*chunks) <= UM_CHUNK_ARRAY_INTERNAL_POINTERS);
		GC_UM_Array_Chunk anode = NULL, prevnode = NULL;
		for (int i = 0 ; i < getLengthOfList(*chunks) ; i++) {
			POPCHUNK(*chunks, anode);
			if (prevnode == NULL)
				prevnode = anode;
			else {
				prevnode->next_chunk = anode;
				prevnode = anode;
			}
			anode->array_chunk_length = numElements;
			anode->array_height = height;
			anode->array_chunk_ml_header = header;
			anode->array_chunk_header |= UM_CHUNK_IN_USE;
			anode->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
			anode->next_chunk = NULL;
			anode->parent = root;
			anode->root = rootroot;
			root->ml_array_payload.um_array_pointers[i] = anode;
		}

		return root;
	}

	/* otherwise H>0, link enough chunks to our root to fully populate it */

	for (int i = 0 ; i < UM_CHUNK_ARRAY_INTERNAL_POINTERS ; i++) {
		GC_UM_Array_Chunk anode = NULL;
		POPCHUNK(*chunks, anode);
		anode->root = rootroot;
		anode->parent = root;
		anode->array_chunk_length = numElements;
		anode->array_chunk_ml_header = header;
		anode->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;
		anode->array_chunk_header |= UM_CHUNK_IN_USE;
		root->ml_array_payload.um_array_pointers[i] = anode;
		create_array_tree(anode,
					 	  chunks,
						  numChunks,
						  height-1,
						  header,
						  numElements,
						  rootroot);
	}
	return root;

}

#define DEBUG_MEM 0
#define DEBUG_CHUNK_ARRAY 0
#if GC_MODEL_OBJPTR_SIZE == 32
# define POW ipow
#else
# define POW pow
#endif

/* this is used by I/O (runtime/basis/...) routines. given an array
 * chunk, return the first leaf. given a leaf, return the next leaf
 * or null of there are no more. return the total number of elements in the
 * array. the caller can use this, and the number of times they've called
 * this routine, to calculate "number of elements left to process"
 *
 * the basis routines dont have convenient access to the runtime
 * header files, so this is why we do this here.
 */
pointer
UM_walk_array_leafs(pointer _c, size_t *nels)
{
	GC_UM_Array_Chunk c = (GC_UM_Array_Chunk)(_c - GC_HEADER_SIZE - sizeof(Word32_t));

	// this isnt an array, probably a global static string
	if (c->array_chunk_magic != UM_ARRAY_SENTINEL) {
		if (nels) nels = 0;
		return _c;
	}

	bool first_time = FALSE;
	while (c->array_chunk_type == UM_CHUNK_ARRAY_INTERNAL) {
		if (nels) {
			if (c->root) *nels = c->root->num_els;
			else *nels = c->num_els;
		}
   		c = c->ml_array_payload.um_array_pointers[0];
		first_time = TRUE;
   	}

	assert (c->array_chunk_type == UM_CHUNK_ARRAY_LEAF);

	if (first_time) {
		return (pointer) &(c->ml_array_payload.ml_object[0]);
   	}

	if (c->next_chunk)
		return (pointer) &(c->next_chunk->ml_array_payload.ml_object[0]);

	return NULL;
}

__attribute__((unused))
pointer GC_arrayAllocate(GC_state s,
						 __attribute__ ((unused)) size_t ensureBytesFree,
						 GC_arrayLength numElements,
						 GC_header header) {
	size_t bytesPerElement = 0;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;

	/* calc number of leaves */

	splitHeader(s, header, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
	bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);

	size_t numElsPerChunk = UM_CHUNK_ARRAY_PAYLOAD_SIZE / bytesPerElement;
	size_t numLeaves = numElements / numElsPerChunk + (numElements % numElsPerChunk != 0);

	/* calc height of tree */

	size_t treeHeight = ceil(log(numLeaves) / log(UM_CHUNK_ARRAY_INTERNAL_POINTERS));

	/* calc total number of chunks needed */

	size_t numChunks = (POW(UM_CHUNK_ARRAY_INTERNAL_POINTERS,treeHeight+1)-1) / (UM_CHUNK_ARRAY_INTERNAL_POINTERS-1);

	if (DEBUG_MEM) {
		fprintf(stderr, "%d] GC_arrayAllocate: numElements: %zd (%zd b/e), numElsPerChunk: %zd, numChunks: %zd\n",
				PTHREAD_NUM, numElements, bytesPerElement, numElsPerChunk, numChunks);
	}

	assert (numChunks > 0);

	/* reserve chunks: will block if there aren't enough chunks */

	reserveAllocation(s, numChunks);
	GC_UM_Array_Chunk allocHead = allocateArrayChunks(s, &(s->umheap), numChunks);
	assert (getLengthOfList(allocHead) == numChunks);

	if (DEBUG_CHUNK_ARRAY)
		fprintf(stderr, "%d]   Initial allocHead "FMTPTR" length: %zd, numChunks = %zd\n",
			PTHREAD_NUM, (uintptr_t)allocHead,
			getLengthOfList(allocHead),
			numChunks);

	/* allocate root node */

	GC_UM_Array_Chunk root;
	POPCHUNK(allocHead, root);

	root->array_chunk_length = numElements;
	root->num_els = numElements;
	root->el_size = bytesPerElement;
	root->array_chunk_ml_header = header;
	root->array_chunk_type = UM_CHUNK_ARRAY_LEAF;
	root->num_els_per_chunk = numElsPerChunk;
	root->array_height = treeHeight;
	root->parent = NULL;
	root->root = NULL;
	root->array_chunk_header |= UM_CHUNK_IN_USE;

	if (numChunks == 1 || numElements == 0) {
		return (pointer) &(root->ml_array_payload.ml_object[0]);
	}

	/* if we have more than 1 chunk, then this is a tree */

	assert (numChunks > 1 && treeHeight > 0);
	root->array_chunk_type = UM_CHUNK_ARRAY_INTERNAL;

	create_array_tree(root, &allocHead, numChunks, treeHeight-1, header, numElements, root);

	return (pointer) &(root->ml_array_payload.ml_object[0]);
}
