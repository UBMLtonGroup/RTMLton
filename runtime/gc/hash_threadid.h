#define PTHREAD_NUM 0

#if (defined (MLTON_GC_INTERNAL_FUNCS))


void set_pthread_num(uint32_t tid);
uint32_t get_pthread_num();

unsigned char hash_tid(pthread_t id);

#endif /* (defined (MLTON_GC_INTERNAL_FUNCS)) */
