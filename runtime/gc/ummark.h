#if (defined (MLTON_GC_INTERNAL_FUNCS))
void getObjectType(GC_state s, objptr *opp);
void umDfsMarkObjects(GC_state s, objptr *opp, GC_markMode m);
void umDfsMarkObjectsUnMark(GC_state s, objptr *opp);
void umDfsMarkObjectsMark(GC_state s, objptr *opp);
void markUMArrayChunks(GC_state s, GC_UM_Array_Chunk p, GC_markMode m);

static void markChunk(pointer p, GC_objectTypeTag tag,GC_markMode m,GC_state s,uint16_t numObjptrs);
static void umShadeObject(GC_state s,objptr *opp);
#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
