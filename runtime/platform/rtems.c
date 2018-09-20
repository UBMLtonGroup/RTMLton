#include "platform.h"
#include "diskBack.unix.c"
//#include "displayMem.proc.c"
//#include "mmap-protect.c"
//#include "nonwin.c"
//#include "use-mmap.c"
static int RTEMS_ROUNDING_MODE = 0;
static const int MEGABYTES = 1024 * 1024;
#ifdef GC_mmapAnon
#undef GCmmappAnon
#endif
#ifdef GC_release
#undef GC_release
#endif
#ifdef GC_displayMem
#undef GC_displayMem
#endif
#ifdef GC_physMem
#undef GC_physMem
#endif

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

#ifdef __sparc__

int fegetround(void) {
  return RTEMS_ROUNDING_MODE;
}

int fesetround(int rounding_mode) {
    RTEMS_ROUNDING_MODE = rounding_mode;
    return 0;
}

/* this is just as ad-hoc as the last time around, but _SC_PHYS_PAGES
 * should work a bit better. */

/*size_t GC_pageSize (void) {
        //returns 4096 because the function isn't properly implemented :(
        return (size_t)getpagesize();
}

uintmax_t GC_physMem (void) {
        return _SC_PHYS_PAGES;
}*/

#endif /* sparc stuff */

//#include "diskBack.unix.c"
//#include "sysconf.c"
  
