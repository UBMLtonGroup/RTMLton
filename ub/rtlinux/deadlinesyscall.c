#define _GNU_SOURCE
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

# define timespec_diff_macro(a, b, result)                  \
  do {                                                \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;  \
    if ((result)->tv_nsec < 0) {                      \
      --(result)->tv_sec;                             \
      (result)->tv_nsec += 1000000000;                \
    }                                                 \
  } while (0)

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

#define SCHED_DEADLINE  6
#define SCHED_FLAG_DL_OVERRUN		0x04

/* __NR_sched_setattr number */
#ifndef __NR_sched_setattr
#ifdef __x86_64__
#define __NR_sched_setattr      314
#endif

#ifdef __i386__
#define __NR_sched_setattr      351
#endif

#ifdef __arm__
#define __NR_sched_setattr      380
#endif

#ifdef __aarch64__
#define __NR_sched_setattr      274
#endif
#endif

/* __NR_sched_getattr number */
#ifndef __NR_sched_getattr
#ifdef __x86_64__
#define __NR_sched_getattr      315
#endif

#ifdef __i386__
#define __NR_sched_getattr      352
#endif

#ifdef __arm__
#define __NR_sched_getattr      381
#endif

#ifdef __aarch64__
#define __NR_sched_getattr      275
#endif
#endif

static volatile int done;


struct sched_attr {
    __u32 size;

    __u32 sched_policy;
    __u64 sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    __s32 sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    __u32 sched_priority;

    /* SCHED_DEADLINE */
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};

int sched_setattr(pid_t pid,
              const struct sched_attr *attr,
              unsigned int flags)
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

int sched_getattr(pid_t pid,
              struct sched_attr *attr,
              unsigned int size,
              unsigned int flags)
{
    return syscall(__NR_sched_getattr, pid, attr, size, flags);
}

void sig_handler(int signo)
{
    printf("received signal %d\n", signo);
}


/* from "man sched"

           arrival/wakeup                    absolute deadline
                |    start time                    |
                |        |                         |
                v        v                         v
           -----x--------xooooooooooooooooo--------x--------x---
                         |<-- Runtime ------->|
                |<----------- Deadline ----------->|
                |<-------------- Period ------------------->|

       The kernel requires that:

           sched_runtime <= sched_deadline <= sched_period
*/

void *run_deadline(void *data)
 {
    struct sched_attr attr;
    int x = 0, ret;
    unsigned int flags = 0;
    struct timespec prev, cur, diff;
    struct timespec runtime_stop, runtime;

    printf("deadline thread start %ld\n", (long int) gettid());

    attr.size = sizeof(attr);
    attr.sched_flags = SCHED_FLAG_DL_OVERRUN;
    attr.sched_nice = 0;
    attr.sched_priority = 0;

    attr.sched_policy   = SCHED_DEADLINE;
    attr.sched_runtime  = 25000; //5 * 1000 * 1000;
    attr.sched_period   = 1000 * 1000 * 1000;
    attr.sched_deadline = 250 * 1000 * 1000;

    printf("Setting schedule to (ns): runtime=%lld deadline=%lld period=%lld\n\n",
            (long long int)attr.sched_runtime, (long long int)attr.sched_deadline, (long long int)attr.sched_period);
    ret = sched_setattr(0, &attr, flags);
    if (ret < 0) {
        done = 0;
        perror("sched_setattr");
        exit(-1);
    }

    clock_gettime(CLOCK_MONOTONIC, &prev);
    printf("time (s) -- actual period (s) -- actual runtime (ns)\n");
    while (!done) {
        clock_gettime(CLOCK_MONOTONIC, &cur);
        timespec_diff_macro(&cur, &prev, &diff);
        memcpy(&prev, &cur, sizeof(struct timespec));

        printf("%lld.%.9ld -- ", (long long)prev.tv_sec, prev.tv_nsec);
        printf("%lld.%.9ld -- ", (long long)diff.tv_sec, diff.tv_nsec);
        x++;
        clock_gettime(CLOCK_MONOTONIC, &runtime_stop);
        timespec_diff_macro(&runtime_stop, &cur, &runtime);
        printf("%9ld\n",  runtime.tv_nsec);

        sched_yield();
    }
    return NULL;
 }

 int main (int argc, char **argv)
 {
    pthread_t thread;

    /* this is supposed to be delivered if there is a runtime over-run */

    if (signal(SIGXCPU, sig_handler) == SIG_ERR)
        perror("can't catch SIGXCPU");

    printf("main thread [%ld]\n", (long int) gettid());
    pthread_create(&thread, NULL, run_deadline, NULL);
    sleep(10);
    done = 1;
    pthread_join(thread, NULL);
    printf("main dies [%ld]\n", (long int)gettid());
    return 0;
 }
