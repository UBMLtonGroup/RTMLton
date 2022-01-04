#include <inttypes.h>
#include <stdint.h>
#ifdef __UCLIBC__
#include <fpu_control.h>
#else
#include <fenv.h>
#endif

#include <unistd.h>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <syslog.h>
#include <termios.h>
#include <utime.h>

#ifdef RTLINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
              unsigned int flags);
int sched_getattr(pid_t pid,
              struct sched_attr *attr,
              unsigned int size,
              unsigned int flags);
#endif /* RTLINUX */

#ifdef __UCLIBC__
#define HAS_FEROUND FALSE
#else
#define HAS_FEROUND TRUE
#endif
#define HAS_MSG_DONTWAIT TRUE
#define HAS_REMAP TRUE
#define HAS_SIGALTSTACK TRUE
#define HAS_SPAWN FALSE
#define HAS_TIME_PROFILING TRUE

#define MLton_Platform_OS_host "linux"

// environ is already defined if _GNU_SOURCE is.
#if !defined(_GNU_SOURCE) && !defined(__ANDROID__)
extern char **environ; /* for Posix_ProcEnv_environ */
#endif

/* The following is compatibility code with older glibc and kernel
   versions. */

#ifndef __suseconds_t_defined
#include <linux/types.h>
typedef __kernel_suseconds_t suseconds_t;
#define __suseconds_t_defined
#endif

#ifdef __GLIBC__
#if __GLIBC__ == 2 && __GLIBC_MINOR__ <= 1
typedef unsigned long int nfds_t;
#endif
#endif

#ifdef __ANDROID__
/* Work around buggy android system libraries */
#undef PRIxPTR
#define PRIxPTR "x"

/* Needed for fetching program counter */
#include <platform/android_ucontext.h>

/* Android is missing these methods: */
#undef tcdrain
#undef ctermid
#define tcdrain MLton_tcdrain
#define ctermid MLton_ctermid

static inline int tcdrain(int fd) {
  return ioctl(fd, TCSBRK, 1);
}

static inline char* ctermid(char* x) {
  static char buf[] = "/dev/tty";
  if (x) {
    strcpy(x, buf);
    return x;
  } else {
    return &buf[0];
  }
}

#endif

#ifndef SO_ACCEPTCONN
#define SO_ACCEPTCONN 30
#endif

#ifdef __UCLIBC__
#define FE_DOWNWARD     _FPU_RC_DOWN
#define FE_TONEAREST    _FPU_RC_NEAREST
#define FE_TOWARDZERO   _FPU_RC_ZERO
#define FE_UPWARD       _FPU_RC_UP
#endif
