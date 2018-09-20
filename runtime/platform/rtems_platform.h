#ifndef RTEMS_H_
#define RTEMS_H_

#include <rtems.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <bsp.h>
#include <sys/dirent.h>
#include <sys/termios.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/utime.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <rtems/score/resource.h>
#include <sys/syslog.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/sysconf.h>
//#include <sys/sysctl.h>
#include <unistd.h>
//#include <process.h>
#include <inttypes.h>
#include <sys/poll.h>

#define PRIuMAX "llu"
#define PRIxMAX "llx"
#define PRIx32 "d"
#define PRIu32 "u"
#define UINTPTR_MAX  UINT32_MAX
#define PRIxPTR "x"
#define PRIdMAX "d"
#define PRIx16  "d"
#define PRIu16  "d"
#define PRIu8   "d"
#define PRId32  "d"

typedef unsigned long MLton_Rlim_t;
typedef unsigned int nfds_t;
typedef MLton_Rlim_t rlim_t;

#define HAS_FEROUND FALSE
#define HAS_REMAP FALSE
#define HAS_SIGALTSTACK FALSE
#define NEEDS_SIGALTSTACK_EXEC FALSE
#define HAS_SPAWN TRUE
#define HAS_TIME_PROFILING FALSE
#define HAS_MSG_DONTWAIT FALSE

#define MLton_Platform_OS_host "rtems"

struct MLton_pollfd {
        short events;
        int fd;
        short revents;
};
#undef pollfd
#define pollfd MLton_pollfd
//#define POLLIN 1
//#define POLLPRI 2
//#define POLLOUT 4

struct MLton_flock {
        off_t l_len;
        pid_t l_pid;
        off_t l_start;
        short l_type;
        short l_whence;
};
#undef flock
#define flock MLton_flock

#ifndef RLIMIT_CPU
#define RLIMIT_CPU      0               /* CPU time in seconds */
#endif

#ifndef RLIMIT_FSIZE
#define RLIMIT_FSIZE    1               /* Maximum filesize */
#endif


#ifndef RLIMIT_DATA
#define RLIMIT_DATA     2               /* max data size */
#endif

#ifndef RLIMIT_STACK
#define RLIMIT_STACK    3               /* max stack size */
#endif

#ifndef RLIMIT_CORE
#define RLIMIT_CORE     4               /* max core file size */
#endif

#ifndef RLIMIT_NOFILE
#define RLIMIT_NOFILE   5               /* max number of open files */
#endif

#ifndef RLIMIT_OFILE
#define RLIMIT_OFILE    RLIMIT_NOFILE   /* BSD name */
#endif

#ifndef RLIMIT_AS
#define RLIMIT_AS       6               /* address space (virt. memory) limit */
#endif

#define RLIMIT_NLIMITS  7               /* upper bound of RLIMIT_* defines */
#define RLIM_NLIMITS    RLIMIT_NLIMITS

#ifndef RLIM_INFINITY
#define RLIM_INFINITY   (0xffffffffUL)
#endif

#ifndef RLIM_SAVED_MAX
#define RLIM_SAVED_MAX  RLIM_INFINITY
#endif

#ifndef RLIM_SAVED_CUR
#define RLIM_SAVED_CUR  RLIM_INFINITY
#endif

typedef unsigned long MLton_rlim_t;
#undef rlim_t
#define rlim_t MLton_rlim_t

struct MLton_rlimit {
        rlim_t  rlim_cur;
        rlim_t  rlim_max;
};
#undef rlimit
#define rlimit MLton_rlimit

int MLton_getrlimit (int resource, struct rlimit *rlim);
int MLton_setrlimit (int resource, const struct rlimit *rlim);
#undef getrlimit
#undef setrlimit
#define getrlimit MLton_getrlimit
#define setrlimit MLton_setrlimit

#ifdef __sparc__

#ifdef HAS_FEROUND
#undef HAS_FEROUND
#define HAS_FEROUND TRUE
#endif

#undef fegetround
#undef fesetround
#define fegetround MLton_fegetround
#define fesetround MLton_fesetround
#define FE_TONEAREST  0
#define FE_DOWNWARD   1
#define FE_UPWARD     2
#define FE_TOWARDZERO 3
int fegetround(void);
int fesetround(int rounding_mode);
#endif
//#define SPAWN_MODE _P_NOWAIT

#endif
