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

#include "diskBack.unix.c"
//#include "sysconf.c"