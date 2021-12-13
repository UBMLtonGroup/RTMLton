#include "platform.h"

C_Errno_t(C_Int_t) Posix_ProcEnv_setgroups (C_Int_t i, Vector(C_GId_t) v) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return setgroups (i, (const gid_t*)v);
#endif
}
