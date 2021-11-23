#include "platform.h"

C_Errno_t(C_Int_t) Posix_FileSys_Dirstream_closeDir (C_DirP_t p) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return closedir ((DIR *) p);
#endif
}

C_Errno_t(C_DirP_t) Posix_FileSys_Dirstream_openDir (NullString8_t p) {
#if defined(__rtems__)
  return (C_DirP_t)NULL;
#else
  DIR *res = opendir ((const char *) p);
  return (C_Errno_t(C_DirP_t))res;
#endif
}

C_Errno_t(C_String_t) Posix_FileSys_Dirstream_readDir (C_DirP_t d) {
#if defined(__rtems__)
  return (C_String_t)NULL;
#else
  struct dirent *e;
  char *res;

  e = readdir ((DIR *) d);
  res = (NULL == e) ? NULL : e->d_name;
  return (C_Errno_t(C_String_t))res;
#endif
}

void Posix_FileSys_Dirstream_rewindDir (C_DirP_t p) {
#if defined(__rtems__)
  return;
#else
  rewinddir ((DIR *) p);
#endif
}
