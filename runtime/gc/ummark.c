

#define LOCK_WL IFED(pthread_mutex_lock(&s->wl_lock))
#define UNLOCK_WL IFED(pthread_mutex_unlock(&s->wl_lock))


void umDfsMarkObjectsUnMark(GC_state s, objptr *opp) {
	umDfsMarkObjects(s, opp, UNMARK_MODE);
}

void umDfsMarkObjectsMark(GC_state s, objptr *opp) {
	umDfsMarkObjects(s, opp, MARK_MODE);
}

void umDfsMarkObjectsMarkToWL(GC_state s, objptr *opp) {

	umDfsMarkObjectsToWorklist(s, opp, MARK_MODE);
}


void addToWorklist(GC_state s, objptr *opp) {
	LOCK_WL;


	assert(isObjptr(*opp));

	/*Objptr on Worklist is not shaded when :
	 * 1. its pointing to object on old heap
	 * 2. its pointing to a stack or weak object
	 * For now this assertion will return true for these elements. But these elements
	 * will not be collected by the collector(since on old heap) so its ok to let it pass the assertion
	 * TODO: Handle objects on oldheap and stacks */

	assert(isObjectShaded(s, opp) || isObjectMarked(s, opp));

	if (s->wl_length >= s->wl_size) {
		if (1 || DEBUG_RTGC_MARKING)
			fprintf(stderr, "%d] addToWorklist: Allocating more space to worklist\n", PTHREAD_NUM);
		GC_worklist new = malloc(s->wl_size * 2 * sizeof(objptr * ));
		GC_worklist old = s->worklist;
		s->worklist = new;
		memcpy(new, old, s->wl_size * sizeof(objptr * ));
		s->wl_size *= 2;

		if (DEBUG_RTGC_MARKING) {
			fprintf(stderr, "old: "
			FMTPTR
			" \n", (uintptr_t) old);
			fprintf(stderr, "new: "
			FMTPTR
			" \n", (uintptr_t) new);
			fprintf(stderr, "worklist: "
			FMTPTR
			" \n", (uintptr_t) s->worklist);

		}

		free(old);
	}

	s->worklist[s->wl_length] = opp;
	s->wl_length++;

	UNLOCK_WL;
}


bool isEmptyWorklist(GC_state s) {
	if (s->wl_length > 0)
		return false;
	else
		return true;

}


bool isObjectShaded(GC_state s, objptr *opp) {
	pointer p = objptrToPointer(*opp, s->umheap.start);
	GC_header *headerp = getHeaderp(p);
	GC_header header = *headerp;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;
	GC_objectTypeTag tag = ERROR_TAG;
	splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

	if (!isObjectOnUMHeap(s, p))
		return true;
	else
		return isContainerChunkMarkedByMode(p, GREY_MODE, tag);
}

bool isObjectMarked(GC_state s, objptr *opp) {
	pointer p = objptrToPointer(*opp, s->umheap.start);
	GC_header *headerp = getHeaderp(p);
	GC_header header = *headerp;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;
	GC_objectTypeTag tag = ERROR_TAG;
	splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);


	return isContainerChunkMarkedByMode(p, MARK_MODE, tag);
}


static
void umShadeObject(GC_state s, objptr *opp) {

	pointer p = objptrToPointer(*opp, s->umheap.start);

	/*Shade the object only if it is on the UM heap. If not it doesn't
   * matter if the object is shaded or not since GC 
   * will not collect it. This check will also ensure we aren't trying 
   * to shade if *opp has been cleared by mutator (is null) */
	if (isObjectOnUMHeap(s, p)) {
		GC_header *headerp = getHeaderp(p);
		GC_header header = *headerp;
		uint16_t bytesNonObjptrs = 0;
		uint16_t numObjptrs = 0;
		GC_objectTypeTag tag = ERROR_TAG;
		splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

		markChunk(p, tag, GREY_MODE, s, numObjptrs);
	}


}

GC_objectTypeTag getObjectType(GC_state s, pointer p) {
	//pointer p = objptrToPointer(*opp, s->umheap.start);
	GC_header *headerp = getHeaderp(p);
	GC_header header = *headerp;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;
	GC_objectTypeTag tag = ERROR_TAG;
	splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

	return tag;

}

bool isWorklistShaded(GC_state s) {
	/*Enforce invariant:
		* Mutator holds no white references*/

	LOCK_WL;
	int i;

	for (i = 0; i < s->wl_length; i++) {
		if (isObjptr(*(s->worklist[i])) && !isObjectShaded(s, s->worklist[i])) {
			UNLOCK_WL;
			if (DEBUG_RTGC_MARKING)
				fprintf(stderr, "%d] %d th element of worklist has not been shaded\n", PTHREAD_NUM, i);
			return false;
		}
	}
	UNLOCK_WL;

	return true;
}


static void markWorklist(GC_state s) {
	int i, tmplength;

	LOCK_WL;

	if (DEBUG_RTGC_MARKING)
		fprintf(stderr, "%d] markWorkList: Started\n", PTHREAD_NUM);

	tmplength = s->wl_length;
	for (i = 0; i < s->wl_length; i++) {

		if (s->worklist[i] != NULL) {
			/*Call only if item is obptr. This is checked when creating worklist
			 * but possibility of changing when mutator runs*/
			callIfIsObjptr(s, umDfsMarkObjectsMark, s->worklist[i]);
			//umDfsMarkObjectsMark(s, s->worklist[i]);
		}

		/*Remove from worklist if:
		 * 1. item is not an objptr (deleted by mutator)
		 * 2. or If item has been marked*/
		if (!isObjptr(*(s->worklist[i])) || isObjectMarked(s, s->worklist[i]))
			s->worklist[i] = NULL;


		tmplength--;
	}

	s->wl_length = tmplength;

	/*All items in the worklist have been marked*/
	assert(s->wl_length == 0);

	if (DEBUG_RTGC_MARKING)
		fprintf(stderr, "%d] %d markWorkList: Done\n", PTHREAD_NUM, i);


	UNLOCK_WL;


}

static
void markChunk(pointer p, GC_objectTypeTag tag, GC_markMode m, GC_state s, uint16_t numObjptrs) {
	if (tag == NORMAL_TAG || tag == STACK_TAG) {

		if (p >= s->umheap.start &&
			p < (s->umheap.start + s->umheap.size)) /*if object is on UM heap */
		{
			GC_UM_Chunk pchunk;
			if (tag == NORMAL_TAG)
				pchunk = (GC_UM_Chunk)(p - GC_NORMAL_HEADER_SIZE); /*Get the chunk holding the mlton object*/
			else
				pchunk = (GC_UM_Chunk)(p);
			if (m == MARK_MODE) {
				pchunk->chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/
				pchunk->chunk_header |= UM_CHUNK_MARK_MASK;  /*mark chunk header*/
				s->cGCStats.numChunksMarked += 1;
			} else if (m == GREY_MODE) {
				/*shade chunk only if not shaded*/
				if (!(ISGREY(pchunk->chunk_header)) && !(ISMARKED(pchunk->chunk_header))) {
					pchunk->chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/

					pchunk->chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
				}
			} else {
				/*Dont ever run Unmark mode*/
				die("Why are you unmarking from markChunks?\n");

				pchunk->chunk_header &= ~UM_CHUNK_MARK_MASK; /*leave chunk header as it is*/
			}

			if (DEBUG_DFS_MARK) {
				fprintf(stderr, "umDfsMarkObjects: chunk: "
				FMTPTR
				", sentinel: %d,"
				" mark_mode: %d, objptrs: %d\n", (uintptr_t) pchunk,
						pchunk->sentinel, m, numObjptrs);
			}

			if (tag == NORMAL_TAG) {
				/*Mark linked chunk if there is a linked chunk*/
				if (NULL != pchunk->next_chunk) {
					if (m == MARK_MODE) {
						pchunk->next_chunk->chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/

						pchunk->next_chunk->chunk_header |= UM_CHUNK_MARK_MASK;
						s->cGCStats.numChunksMarked += 1;
					} else if (m == GREY_MODE) {
						/*shade chunk only if not shaded*/
						if (!(ISGREY(pchunk->next_chunk->chunk_header)) &&
							!(ISMARKED(pchunk->next_chunk->chunk_header))) {
							pchunk->next_chunk->chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/
							pchunk->next_chunk->chunk_header |= UM_CHUNK_GREY_MASK;  /*shade chunk header*/
						}
					} else {
						pchunk->next_chunk->chunk_header &= ~UM_CHUNK_MARK_MASK;
					}
				}
			}
		}
	} else if (tag == ARRAY_TAG &&
			   p >= s->umheap.start &&
			   p < s->umheap.start + s->umheap.size) {
		GC_UM_Array_Chunk fst_leaf = (GC_UM_Array_Chunk)
				(p - GC_HEADER_SIZE - GC_HEADER_SIZE);
		if (DEBUG_DFS_MARK) {
			fprintf(stderr, "umDfsMarkObjects: marking array: %p, markmode: %d, "
							"magic: %d, length: %d\n", (void *) fst_leaf, m,
					fst_leaf->array_chunk_magic, fst_leaf->array_chunk_length);
		}

		if (fst_leaf->array_num_chunks > 1 &&
			fst_leaf->array_chunk_length > 0) {
			GC_UM_Array_Chunk root = fst_leaf->root;
//            size_t length = root->array_chunk_length;
//
//            size_t i, j;
//            size_t elem_size = bytesNonObjptrs + numObjptrs * OBJPTR_SIZE;
//            for (i=0; i<length; i++) {
//                pointer pobj = UM_Array_offset(s, p, i, elem_size, 0) +
//                    bytesNonObjptrs;
//
//                for (j=0; j<numObjptrs; j++) {
//                    if (m == MARK_MODE)
//                        foreachObjptrInObject(s, pobj, umDfsMarkObjectsMark, true);
//                    else
//                        foreachObjptrInObject(s, pobj, umDfsMarkObjectsUnMark, true);
//                    pobj += OBJPTR_SIZE;
//                }
//            }
			markUMArrayChunks(s, root, m);
		} else
			markUMArrayChunks(s, fst_leaf, m);
	} else {
		if (0) {
			switch (tag) {
				case STACK_TAG:
					fprintf(stderr, "%d]Trying to mark Stack Chunk\n", PTHREAD_NUM);
					break;

				case WEAK_TAG:
					fprintf(stderr, "%d]Trying to mark Weak Chunk\n", PTHREAD_NUM);
					break;

				case ERROR_TAG:
					fprintf(stderr, "%d]Trying to mark chunk with Error tag\n", PTHREAD_NUM);
					break;

				case NORMAL_TAG:
					fprintf(stderr, "%d]Trying to mark Normal Chunk NOT on UMheap \n", PTHREAD_NUM);
					break;

				case ARRAY_TAG:
					fprintf(stderr, "%d]Trying to mark Array Chunk NOT on UMheap\n", PTHREAD_NUM);
					break;

				default:
					die("Trying to mark Chunk with garbage tag\n");
			}
		}
	}


}


static
bool isChunkMarked(pointer p, GC_objectTypeTag tag) {
	/*Treat shaded objects as unmarked*/
	fprintf(stderr, "hi2 %d\n", tag);

	if (tag == NORMAL_TAG) {
		GC_UM_Chunk pc = (GC_UM_Chunk)(p - GC_NORMAL_HEADER_SIZE); /*Get the chunk holding the mlton object*/
		fprintf(stderr, "%x\n", (unsigned int)pc);
		return (ISINUSE(pc->chunk_header) && ISMARKED(pc->chunk_header));
	} else if (tag == STACK_TAG) {

		GC_UM_Chunk pc = (GC_UM_Chunk)(p); /*Get the chunk holding the mlton object*/
		fprintf(stderr, "%x\n", (unsigned int)pc);
		return (ISINUSE(pc->chunk_header) && ISMARKED(pc->chunk_header));

	} else if (tag == ARRAY_TAG) {
		GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)(p - GC_HEADER_SIZE - GC_HEADER_SIZE);

		return (ISINUSE(pc->array_chunk_header) && ISMARKED(pc->array_chunk_header));
	} else {
		/*weak or error tags*/
		die("Why are you checking a %s object chunk??\n",(tag == STACK_TAG)?"Stack":"Weak");
	}

}


static bool isChunkShaded(pointer p, GC_objectTypeTag tag) {
	/*Treat shaded objects as unmarked*/
fprintf(stderr, "hi1 %d\n", tag);
	if (tag == NORMAL_TAG) {
		GC_UM_Chunk pc = (GC_UM_Chunk)(p - GC_NORMAL_HEADER_SIZE); /*Get the chunk holding the mlton object*/

		fprintf(stderr, "%x\n", (unsigned int)pc);

		if (ISINUSE(pc->chunk_header) && ISGREY(pc->chunk_header))
			return true;
		else
			return false;

	} else if (tag == STACK_TAG) {
		GC_UM_Chunk pc = (GC_UM_Chunk)(p); /*Get the chunk holding the mlton object*/

		fprintf(stderr, "%x\n", (unsigned int)pc);

		if (ISINUSE(pc->chunk_header) && ISGREY(pc->chunk_header))
			return true;
		else
			return false;

	} else if (tag == ARRAY_TAG) {
		GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)(p - GC_HEADER_SIZE - GC_HEADER_SIZE);

		if (ISINUSE(pc->array_chunk_header) && ISGREY(pc->array_chunk_header))
			return true;
		else
			return false;

	} else if (tag == WEAK_TAG) {
		die("Why are you checking a %s object chunk??\n", (tag == STACK_TAG) ? "Stack" : "Weak");
	} else {
		return false;
	}

}


static
bool isContainerChunkMarkedByMode(pointer p, GC_markMode m, GC_objectTypeTag tag) {
	switch (m) {
		case MARK_MODE:
			return isChunkMarked(p, tag);
		case UNMARK_MODE:
			return not
			isChunkMarked(p, tag);
		case GREY_MODE:
			return isChunkShaded(p, tag);
		default:
			die("bad mark mode %u", m);
	}
}


//TODO: handle marking the mlton objects if packing more than one object in a chunk
/* Tricolor abstraction at the chunk level. Binary marking for the MLton objects remain same. 
 * Implementation: 
 * 1. If function is in marking mode, mark current chunk grey.
 * 2. mark children grey
 * 3. Mark Chunk Black
 * 4. Continue marking in dfs
 * NOTE: Function shades object grey only if it isn't already marked grey / black. This makes sure that the shading is to a darker shade and never to lighter shade (unless you are in unmark mode) 
 * */

void umDfsMarkObjects(GC_state s, objptr *opp, GC_markMode m) {
	pointer p = objptrToPointer(*opp, s->umheap.start);
	if (DEBUG_DFS_MARK)
		fprintf(stderr, "umDFSMarkObjects: original obj: 0x%x, obj: 0x%x\n",
				(uintptr_t) * opp, (uintptr_t) p);
	GC_header *headerp = getHeaderp(p);
	GC_header header = *headerp;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;
	GC_objectTypeTag tag = ERROR_TAG;
	splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

//    if (DEBUG_DFS_MARK)
	//getObjectType(s, opp);



	/* Using MLton object to track if containing chunk marked */
	if (isContainerChunkMarkedByMode(p, m, tag)) {
		if (DEBUG_DFS_MARK)
			fprintf(stderr, FMTPTR
		"marked by mark_mode: %d, RETURN\n",
				(uintptr_t) p,
				(m == MARK_MODE));
		return;
	}

	/*ensure the MLton object isn't marked. It should be unmarked always*/
	assert(!isPointerMarkedByMode(p, MARK_MODE));

	/*mark children*/
	if (m == MARK_MODE) {
		markChunk(p, tag, GREY_MODE, s, numObjptrs);
		if (numObjptrs > 0)
			foreachObjptrInObject(s, p, umShadeObject, false);
	}


	/*mark object*/
	/*
	 if (m == MARK_MODE) {
		 if (DEBUG_DFS_MARK)
			 fprintf(stderr, FMTPTR" mark b pheader: %x, header: %x\n",
					 (uintptr_t)p, *(getHeaderp(p)), header);

		 header = header | MARK_MASK;
		 *headerp = header;

		 if (DEBUG_DFS_MARK)
			 fprintf(stderr, FMTPTR" mark a pheader: %x, header: %x\n",
					 (uintptr_t)p, *(getHeaderp(p)), header);
	 } else {
		 if (DEBUG_DFS_MARK)
			 fprintf(stderr, FMTPTR" unmark b pheader: %x, header: %x\n",
					 (uintptr_t)p, *(getHeaderp(p)), header);

		 header = header & ~MARK_MASK;
		 (*headerp) = header;

		 if (DEBUG_DFS_MARK)
			 fprintf(stderr, FMTPTR" unmark a pheader: %x, header: %x\n",
					 (uintptr_t)p, *(getHeaderp(p)), header);
	 }
	 */

	/*Mark chunk*/

	markChunk(p, tag, m, s, numObjptrs);


	/*if there are references in the object fields, mark them*/
	if (numObjptrs > 0) {
		if (m == MARK_MODE)
			foreachObjptrInObject(s, p, umDfsMarkObjectsMark, false);
		else {
			/*Unmarking is done by sweep phase of GC.
			 * And no reason to do a dfs grey marking.*/
			die("%d] umDfsMarkObjects: Why are you using %s mode?\n", PTHREAD_NUM,
				(m == UNMARK_MODE) ? "UNMARK" : "GREY");
			foreachObjptrInObject(s, p, umDfsMarkObjectsUnMark, false);
		}
	}
}


void markUMArrayChunks(GC_state s, GC_UM_Array_Chunk p, GC_markMode m) {
	if (DEBUG_DFS_MARK)
		fprintf(stderr, "markUMArrayChunks: %p: marking array markmode: %d, "
						"type: %d\n", (void *) p, m,
				p->array_chunk_type);

	if (m == MARK_MODE) {
		p->array_chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/
		p->array_chunk_header |= UM_CHUNK_MARK_MASK;
		s->cGCStats.numChunksMarked += 1;
	} else if (m == GREY_MODE) {       /*shade chunk only if not shaded*/
		if (!(p->array_chunk_header & UM_CHUNK_GREY_MASK) && !(p->array_chunk_header & UM_CHUNK_MARK_MASK)) {
			p->array_chunk_header &= ~UM_CHUNK_RED_MASK; /*Clear red marking*/

			p->array_chunk_header |= UM_CHUNK_GREY_MASK;
		}
	} else {
		die("Why are you unmarking from markUMArrayChunks?\n");
		p->array_chunk_header &= ~UM_CHUNK_MARK_MASK;
	}

	if (p->array_chunk_type == UM_CHUNK_ARRAY_INTERNAL) {
		int i = 0;
		for (i = 0; i < UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
			GC_UM_Array_Chunk pcur = p->ml_array_payload.um_array_pointers[i];
			if (!pcur)
				break;
			markUMArrayChunks(s, pcur, m);
		}
	}
}


void umDfsMarkObjectsToWorklist(GC_state s, objptr *opp, GC_markMode m) {
	pointer p = objptrToPointer(*opp, s->umheap.start);
	if (DEBUG_DFS_MARK)
		fprintf(stderr, "original obj: 0x%x, obj: 0x%x\n",
				(uintptr_t) * opp, (uintptr_t) p);
	GC_header *headerp = getHeaderp(p);
	GC_header header = *headerp;
	uint16_t bytesNonObjptrs = 0;
	uint16_t numObjptrs = 0;
	GC_objectTypeTag tag = ERROR_TAG;

	if (header == 0) {
		fprintf(stderr, "invalid header for obj: 0x%x\n", (uintptr_t) p);
	}

	splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

//    if (DEBUG_DFS_MARK)
	//getObjectType(s, opp);



	/* Using MLton object to track if containing chunk marked */
	if (isContainerChunkMarkedByMode(p, m, tag)) {
		if (DEBUG_DFS_MARK)
			fprintf(stderr, FMTPTR
		"marked by mark_mode: %d, RETURN\n",
				(uintptr_t) p,
				(m == MARK_MODE));
		return;
	}

	/*ensure the MLton object isn't marked. It should be unmarked always*/
	assert(!isPointerMarkedByMode(p, MARK_MODE));

	/*mark children*/
	if (m == MARK_MODE) {
		markChunk(p, tag, GREY_MODE, s, numObjptrs);
		if (numObjptrs > 0)
			foreachObjptrInObject(s, p, umShadeObject, false);
	}


	/*Mark chunk*/

	markChunk(p, tag, m, s, numObjptrs);


	/*if there are references in the object fields, mark them*/
	if (numObjptrs > 0) {
		if (m == MARK_MODE)
			foreachObjptrInObject(s, p, addToWorklist, false);
		else {
			die("%d] umDfsMarkObjects: Why are you using %s mode?\n", PTHREAD_NUM,
				(m == UNMARK_MODE) ? "UNMARK" : "GREY");
			foreachObjptrInObject(s, p, umDfsMarkObjectsUnMark, false);
		}
	}
}
