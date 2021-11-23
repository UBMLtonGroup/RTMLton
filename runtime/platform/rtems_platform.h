#ifndef RTEMS_H_
#define RTEMS_H_

#include <rtems.h>
#include <bsp.h>
#include <sys/dirent.h>

#if defined(__sparc__) || defined(__arm__)
#include <sys/termios.h>
#endif

#include <sys/fcntl.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/utime.h>
#include <sys/resource.h>
#include <sys/syslog.h>
//#include <sys/sysconf.h>
//#include <sys/sysctl.h>
#include <unistd.h>
#include <sys/socket.h>

#if defined(__sparc__) || defined(__arm__)
# include <process.h>
#endif

#define PRIuMAX "llu"
#define PRIxMAX "llx"
#define PRIx32 "d"
#define PRIu32 "u"
#ifdef UINTPTR_MAX
# undef UINTPTR_MAX
#endif
#define UINTPTR_MAX  UINT32_MAX
#if defined(__sparc__)
# define PRIdMAX "d"
# define PRIxPTR "x"
#else
# define PRIdMAX "lld"
# define PRIxPTR "lx"
#endif
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
#define HAS_SPAWN FALSE
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
#define POLLIN 1
#define POLLPRI 2
#define POLLOUT 4

////// rtems i386
#ifndef NCCS
#define NCCS            18
#endif
typedef unsigned char   MLton_cc_t;
typedef unsigned int    MLton_speed_t;
typedef unsigned int    MLton_tcflag_t;

#undef cc_t
#undef speed_t
#undef tcflag_t

#define cc_t MLton_cc_t
#define speed_t MLton_speed_t
#define tcflag_t MLton_tcflag_t

struct MLton_termios {
        cc_t c_cc[NCCS];
        tcflag_t c_cflag;
        tcflag_t c_iflag;
        tcflag_t c_lflag;
        tcflag_t c_oflag;
};
#undef termios
#define termios MLton_termios


//// end rtems i386

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

#define SPAWN_MODE _P_NOWAIT

#endif
