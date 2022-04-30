#ifdef __rtems__
#include <time.h>
#endif
#ifdef RTLINUX
#include <time.h>
#endif

#define NSEC_PER_SEC 1000000000


#ifndef USE_NANO_SLEEP
static int spinsleep(int s, int us);
static int spinsleep(int s, int us) {
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC_RAW, &a);
    double ad = ((double)(a.tv_sec) + ((double)(a.tv_nsec) / NSEC_PER_SEC));
    double interval = ((double)s + ((double)(us*1000)/NSEC_PER_SEC));
    int done = 0;
	if (DEBUG) fprintf(stderr, "spinsleep(%d, %d) for %f\n", s, us, interval);
    while (!done) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &b);
        double bd = ((double)(b.tv_sec) + ((double)(b.tv_nsec) / NSEC_PER_SEC));
        if ((bd-ad) > interval) done = 1;
    }
    return 0;
}
#endif


/* signal free sleep
 * on rtlinux, using nanosleep or select wasn't predictable
 * and measuring the actual time sleeping would vary quite 
 * a bit. so on that platform we call spinsleep (above) whose
 * timing measurements were much more consistent. for a deadline
 * scheduled task, spinning is ok provided the entire schedule
 * validates, and the spin time is factored into the task's runtime
 */
int ssleep(int s, int us)
{
        int ret = 0;
#if defined(__rtems__)
        usleep(s*1000000 + us);
#elif defined(RTLINUX)
 		struct timespec a, b;
#ifdef USE_NANO_SLEEP
        struct timespec ts;
        ts.tv_sec = s;
        ts.tv_nsec = us * 1000;
#endif

        clock_gettime(CLOCK_MONOTONIC_RAW, &a);
#ifdef USE_NANO_SLEEP
        nanosleep(&ts, NULL);
#else
        ret = spinsleep(s, us);
#endif
    	clock_gettime(CLOCK_MONOTONIC_RAW, &b);

        double ad = ((double)(a.tv_sec) + ((double)(a.tv_nsec) / NSEC_PER_SEC));
	double bd = ((double)(b.tv_sec) + ((double)(b.tv_nsec) / NSEC_PER_SEC));
        double dd = bd - ad;
	if (DEBUG) fprintf(stderr, "ssleep>>> %d %d ... %f\n", s, us, dd);

#else
        struct timeval tv;

        if (s == 0 && us == 0) s = 1;

        tv.tv_sec = s;
        tv.tv_usec = us;

        do
        {
          ret = select(1, NULL, NULL, NULL, &tv);
        }
        while((ret == -1)&&(errno == EINTR));
#endif
        return ret;
}
