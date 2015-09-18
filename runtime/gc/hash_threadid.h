#define PTHREAD_NUM get_pthread_num()

uint32_t get_pthread_num(void);

#if (defined (MLTON_GC_INTERNAL_FUNCS))

void set_pthread_num(uint32_t tid);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
