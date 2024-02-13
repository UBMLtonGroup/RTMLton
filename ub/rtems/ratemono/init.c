/*
 * Simple RTEMS configuration
 */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_RTEMS_POSIX_API

//#define CONFIGURE_POSIX_INIT_THREAD_TABLE

#define CONFIGURE_MICROSECONDS_PER_TICK   1000 /* 1 millisecond */
#define CONFIGURE_TICKS_PER_TIMESLICE       50 /* 50 milliseconds */

#define CONFIGURE_MAXIMUM_POSIX_THREADS 6
#define CONFIGURE_MAXIMUM_TASKS           2
#define CONFIGURE_MAXIMUM_PERIODS         1
#define CONFIGURE_INIT_TASK_STACK_SIZE    (RTEMS_MINIMUM_STACK_SIZE * 2)
#define CONFIGURE_INIT_TASK_PRIORITY      10
#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_EXTRA_TASK_STACKS       (1 * RTEMS_MINIMUM_STACK_SIZE)

#define CONFIGURE_MEMORY_OVERHEAD 1

#define CONFIGURE_INIT

//#define CONFIGURE_SCHEDULER_EDF
//#define CONFIGURE_SCHEDULER_NAME rtems_build_name('U', 'E', 'D', 'F')

#include <rtems/confdefs.h>
