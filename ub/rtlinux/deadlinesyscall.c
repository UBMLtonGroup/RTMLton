#define _GNU_SOURCE
#include <linux/kernel.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <linux/types.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <stdlib.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

#define SCHED_DEADLINE  6

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


void *run_deadline(void *data)
 {
     struct sched_attr attr;
     int x = 0, ret;
     unsigned int flags = 0;

     printf("deadline thread start %ld\n",
         gettid());

     attr.size = sizeof(attr);
     attr.sched_flags = 0;
     attr.sched_nice = 0;
     attr.sched_priority = 0;

     /* creates a 10ms/30ms reservation */
     attr.sched_policy = SCHED_DEADLINE;
     attr.sched_runtime = 10 * 1000 * 1000;
     attr.sched_period  = 30 * 1000 * 1000;
     attr.sched_deadline= 30 * 1000 * 1000;

     ret = sched_setattr(0, &attr, flags);
     if (ret < 0) {
         done = 0;
         perror("sched_setattr");
         exit(-1);
     }

     while (!done) {
         x++;
     }
     return NULL;
 }

 int main (int argc, char **argv)
 {
     pthread_t thread;
     printf("main thread [%ld]\n", gettid());
     pthread_create(&thread, NULL, run_deadline, NULL);
     sleep(10);
     done = 1;
     pthread_join(thread, NULL);
     printf("main dies [%ld]\n", gettid());
     return 0;
 }
