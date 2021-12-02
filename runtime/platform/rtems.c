#include "platform.h"

static const int MEGABYTES = 1024 * 1024;

void *GC_mmapAnon (void *start, size_t length) {
  return malloc(length);
}

void GC_release (void *base, size_t length) {
  return;
}

void GC_displayMem(void) {
  
}

uintmax_t GC_physMem (void) {
  return 128 * MEGABYTES;
}

size_t GC_pageSize(void) {
  return 1 * MEGABYTES;
}

#include <stdio.h>
#include <stdarg.h>

#pragma GCC  diagnostic ignored "-Wsuggest-attribute=format"
#pragma GCC  diagnostic ignored "-Wformat-nonliteral"

int __fprintf_chk(FILE * stream, int flag, const char * format, ...) {
  va_list ap;
  int ret;
  va_start (ap, format);
  ret = fprintf (stream, format, ap);
  va_end (ap);
  return ret;
}

int * __errno_location(void) {
  return (int *)&errno;
}


int vprintf_chk(const char *fmt, va_list list) {
  return vprintf(fmt, list);
}

int __vfprintf_chk(FILE *fp, int x, const char *fmt, va_list list) {
  return vfprintf(fp, fmt, list);
}

int vsprintf_chk(char *str, const char *fmt, va_list list) {
  return vsprintf(str, fmt, list);
}

int vsnprintf_chk(char *str, size_t size, const char *fmt, va_list list) {
  return vsnprintf(str, size, fmt, list);
}



#include "diskBack.unix.c"
//#include "sysconf.c"