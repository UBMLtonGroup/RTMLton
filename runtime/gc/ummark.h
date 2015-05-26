#if (defined (MLTON_GC_INTERNAL_FUNCS))
void getObjectType(GC_state s, objptr *opp);
void umDfsMarkObjects(GC_state s, objptr *opp, GC_markMode m);
void umDfsMarkObjectsUnMark(GC_state s, objptr *opp);
void umDfsMarkObjectsMark(GC_state s, objptr *opp);
void markUMArrayChunks(GC_state s, GC_UM_Array_Chunk p, GC_markMode m);
#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
