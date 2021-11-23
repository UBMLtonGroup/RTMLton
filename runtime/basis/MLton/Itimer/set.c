
#if defined(__rtems__)
#define _RLIM_T_DECLARED
#include <rtems.h>
#include <sys/times.h>
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <rtems/seterr.h>
#include <rtems/score/todimpl.h>
#include <rtems/score/timestamp.h>
#include <rtems/score/threadimpl.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <rtems/seterr.h>
#endif
#include "platform.h"

C_Errno_t(C_Int_t) 
MLton_Itimer_set (C_Int_t which,
            C_Time_t interval_tv_sec, 
            C_SUSeconds_t interval_tv_usec,
            C_Time_t value_tv_sec, 
            C_SUSeconds_t value_tv_usec) {
  struct itimerval v;

  v.it_interval.tv_sec = interval_tv_sec;
  v.it_interval.tv_usec = interval_tv_usec;
  v.it_value.tv_sec = value_tv_sec;
  v.it_value.tv_usec = value_tv_usec;
  return setitimer (which, &v, (struct itimerval *)NULL);
}
