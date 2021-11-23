#include "platform.h"

static struct termios Posix_TTY_Termios_termios;

C_TCFlag_t Posix_TTY_Termios_getIFlag (void) {
  return Posix_TTY_Termios_termios.c_iflag;
}

C_TCFlag_t Posix_TTY_Termios_getOFlag (void) {
  return Posix_TTY_Termios_termios.c_oflag;
}

C_TCFlag_t Posix_TTY_Termios_getCFlag (void) {
  return Posix_TTY_Termios_termios.c_cflag;
}

C_TCFlag_t Posix_TTY_Termios_getLFlag (void) {
  return Posix_TTY_Termios_termios.c_lflag;
}

void Posix_TTY_Termios_getCC (Array(C_CC_t) a) {
  for (int i = 0; i < NCCS; i++) 
    ((cc_t*)a)[i] = Posix_TTY_Termios_termios.c_cc[i];
}

C_Speed_t Posix_TTY_Termios_cfGetOSpeed (void) {
#if defined(__rtems__)
  return (C_Speed_t)NULL;
#else
  return cfgetospeed (&Posix_TTY_Termios_termios);
#endif
}

C_Speed_t Posix_TTY_Termios_cfGetISpeed (void) {
#if defined(__rtems__)
  return (C_Speed_t)NULL;
#else
  return cfgetispeed (&Posix_TTY_Termios_termios);
#endif
}

void Posix_TTY_Termios_setIFlag (C_TCFlag_t f) {
  Posix_TTY_Termios_termios.c_iflag = f;
}

void Posix_TTY_Termios_setOFlag (C_TCFlag_t f) {
  Posix_TTY_Termios_termios.c_oflag = f;
}

void Posix_TTY_Termios_setCFlag (C_TCFlag_t f) {
  Posix_TTY_Termios_termios.c_cflag = f;
}

void Posix_TTY_Termios_setLFlag (C_TCFlag_t f) {
  Posix_TTY_Termios_termios.c_lflag = f;
}

void Posix_TTY_Termios_setCC (Array(C_CC_t) a) {
  for (int i = 0; i < NCCS; i++) 
    Posix_TTY_Termios_termios.c_cc[i] = ((cc_t*)a)[i];
}

C_Errno_t(C_Int_t) Posix_TTY_Termios_cfSetOSpeed (C_Speed_t s) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return cfsetospeed (&Posix_TTY_Termios_termios, s);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_Termios_cfSetISpeed (C_Speed_t s) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return cfsetispeed (&Posix_TTY_Termios_termios, s);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_drain (C_Fd_t f) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcdrain (f);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_flow (C_Fd_t f, C_Int_t i) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcflow (f, i);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_flush (C_Fd_t f, C_Int_t i) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcflush (f, i);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_getattr (C_Fd_t f) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcgetattr (f, &Posix_TTY_Termios_termios);
#endif
}

C_Errno_t(C_PId_t) Posix_TTY_TC_getpgrp (C_Fd_t f) {
  return tcgetpgrp (f);
}

C_Errno_t(C_Int_t) Posix_TTY_TC_sendbreak (C_Fd_t f, C_Int_t i) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcsendbreak (f, i);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_setattr (C_Fd_t f, C_Int_t i) {
#if defined(__rtems__)
  return (C_Int_t)NULL;
#else
  return tcsetattr (f, i, &Posix_TTY_Termios_termios);
#endif
}

C_Errno_t(C_Int_t) Posix_TTY_TC_setpgrp (C_Fd_t f, C_PId_t p) {
  return tcsetpgrp (f, p);
}
