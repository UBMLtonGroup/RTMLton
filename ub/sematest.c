// unix% gcc -o sematest sematest.c -lpthread


#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>

 int rchk(double perc) {
    double r = rand();
    if (r / RAND_MAX < perc)
        return 1;
    return 0;
}

pthread_mutex_t lock;
pthread_cond_t cond;

int running_threads = 0;
int gc_needed = 0;

#define LOCK pthread_mutex_lock(&lock)
#define UNLOCK pthread_mutex_unlock(&lock)

void *gc(void *p) {
    printf("GC init\n");

    while (1) {
        LOCK;
        do {
            do {
                pthread_cond_wait(&cond, &lock);  // implicit UNLOCK
            } while (running_threads);
        } while (!gc_needed);

        // at this point the lock has been re-acquired and all threads are paused

        printf("GC running needed=%d threads=%d\n", gc_needed, running_threads);
        sleep(2);
        gc_needed = 0;

        UNLOCK;
    }
}

/*
 * - threads can ask for GC's by setting gc_needed to 1
 * - threads can enter safe point (for a GC) by decrementing
 *   running_threads by one.
 * - A thread may leave a safe point only if gc_needed is zero.
 * - Upon leaving the safe point, running_threads is incremented by one.
 *
 * ask_for_gc:
 *    lock(gc_needed = 1)
 *
 * enter_safe_point:
 *    lock(running_threads --)
 *
 * leave_safe_point:
 *    spin while (gc_needed == 1)
 *    lock(running_threads ++)
 *
 */

#define pthread_yield sched_yield 

#define ASK_FOR_GC do { LOCK; gc_needed = 1; UNLOCK; } while(0)
#define ENTER_SAFEPOINT do { LOCK; running_threads--; pthread_cond_signal(&cond); UNLOCK; } while(0)
#define LEAVE_SAFEPOINT do { while (gc_needed) pthread_yield(); LOCK; running_threads++; UNLOCK; } while(0)

void *comp(void *p) {
    int _p = (int)p;


    LOCK;
    running_threads ++;
    printf("%d] COMP init: running_threads=%d\n", _p, running_threads);
    UNLOCK;
    sleep (1);

    while (1) {
        // randomly ask for a GC
        if (rchk(.25)) {
            printf("%d] randomly asking for a gc\n", _p);
            ASK_FOR_GC;
        }

        // randomly enter safepoint
        // if a GC was requested (by any thread) then LEAVE_SAFEPOINT should
        // spin until the GC is completed. maybe use another sema so we dont have
        // to spin and waste cpu
        if (rchk(.25)) {
            printf("%d] randomly entering safepoint with %d running threads. gc_needed=%d\n",
                   _p, running_threads, gc_needed);
            ENTER_SAFEPOINT;
            printf("%d] past safepoint, now we have %d running threads. leaving safepoint. gc_needed=%d\n",
                   _p, running_threads, gc_needed);
            LEAVE_SAFEPOINT;
            printf("%d] successfully left safepoint, now we have %d running threads. gc_needed=%d\n",
                   _p, running_threads, gc_needed);
        }

        printf("%d] doing work\n", _p);
    }
}


int main() {
    pthread_t p1, p2, p3, p4;

    printf("sema test\n");
    srand(time(NULL));

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&p1, NULL, gc, NULL);
    sleep(1);
    pthread_create(&p2, NULL, comp, (void *)0);
    pthread_create(&p3, NULL, comp, (void *)1);
    pthread_create(&p4, NULL, comp, (void *)2);
    sleep (10);
    return 0;
}
