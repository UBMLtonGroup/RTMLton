#include "platform.h"

C_Int_t Posix_ProcEnv_getgroupsN (void) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return getgroups (0, (gid_t*)NULL);
#endif
}

C_Errno_t(C_Int_t) Posix_ProcEnv_getgroups (C_Int_t i, Array(C_GId_t) a) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return getgroups (i, (gid_t*)a);
#endif
}
