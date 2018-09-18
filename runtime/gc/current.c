/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

objptr getThreadCurrentObjptr (GC_state s) {
#if 0
    if(DEBUG)
        fprintf(stderr,"pthread num = %d, currentThread - %x\n",PTHREAD_NUM,s->currentThread[PTHREAD_NUM]);
#endif 
    return s->currentThread[PTHREAD_NUM];
}

GC_thread getThreadCurrent (GC_state s) {
  pointer p = objptrToPointer(getThreadCurrentObjptr(s), s->heap.start);
  return (GC_thread)(p + offsetofThread (s));
}

objptr getStackCurrentObjptr (GC_state s) {
  GC_thread thread = getThreadCurrent(s);
#if 0
  if(DEBUG)
        fprintf(stderr,"%d]currenThread= %x\n",PTHREAD_NUM,thread);
#endif
  return thread->stack;
}

GC_stack getStackCurrent (GC_state s) {
  pointer p = objptrToPointer(getStackCurrentObjptr(s), s->heap.start);
  return (GC_stack)p;
}


/* note that what is returned is not a GC_stack but a frame.
 * a frame is just a pointer, it has no intrinsic structure.
 * the frame layout structure describes the eventual layout of
 * a given frame, but that is not known until after compilation.
 */
objptr getUMStackCurrentObjptr (GC_state s) {
  GC_thread thread = getThreadCurrent(s);
  return thread->umstack;
}

pointer getUMStackCurrent (GC_state s) {
  pointer p = objptrToPointer(getUMStackCurrentObjptr(s), s->heap.start);
  return p;
}
