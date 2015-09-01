#include <pthread.h>
#include <malloc.h>
#include <stdint.h>


static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

uint32_t
get_pthread_num()
{
        uint32_t *ptr;
        ptr = (uint32_t *)pthread_getspecific(key);
        if (ptr != NULL) return (uint32_t)*ptr;
        return 0;
}


static void
make_key()
{
    (void) pthread_key_create(&key, NULL);
}


void
set_pthread_num(uint32_t tid)
{
    void *ptr;

    (void) pthread_once(&key_once, make_key);
    if ((ptr = pthread_getspecific(key)) == NULL) {
        ptr = (void*)malloc(sizeof(uint32_t));
        *(uint32_t *)ptr = (uint32_t)tid;
        (void) pthread_setspecific(key, ptr);
    }
}

