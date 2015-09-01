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
        if (ptr != NULL) return *ptr;
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
    uint32_t *ptr;

    (void) pthread_once(&key_once, make_key);
    if ((ptr = (uint32_t *)pthread_getspecific(key)) == NULL) {
        ptr = (uint32_t *)malloc(sizeof(uint32_t));
        *ptr = tid;
        (void) pthread_setspecific(key, ptr);
    }
}

