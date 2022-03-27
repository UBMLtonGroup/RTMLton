/*
 * Hello world example
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include "/opt/rtems6/i386-rtems6/include/stdio.h"
#include <rtems/cpuuse.h>

rtems_task Periodic_task(rtems_task_argument argument);
rtems_task Init(rtems_task_argument argument);
volatile int partial_loop = 0;

rtems_task Periodic_task(rtems_task_argument arg)
{
    rtems_name        name;
    rtems_id          period;

    rtems_status_code status;
    name = rtems_build_name( 'P', 'E', 'R', 'D' );
    status = rtems_rate_monotonic_create( name, &period );
    if ( status != RTEMS_SUCCESSFUL ) {
        printf( "rtems_monotonic_create failed with status of %d.\n", status );
        exit( 1 );
    }
    while ( 1 ) {
        int x = rtems_clock_get_ticks_since_boot();
        printf("Calling rtems_rate_monotonic_period(period=250 ticks). clock=%d ticks/sec=%d\n", x, rtems_clock_get_ticks_per_second());

        if ( rtems_rate_monotonic_period( period, 250 ) == RTEMS_TIMEOUT )
            break;
        x = rtems_clock_get_ticks_since_boot();
        printf("  Task awake: periodic action. clock=%d\n", x);
        /* Perform some periodic actions */
    }
    /* missed period so delete period and SELF */
    status = rtems_rate_monotonic_delete( period );
    if ( status != RTEMS_SUCCESSFUL ) {
        printf( "rtems_rate_monotonic_delete failed with status of %d.\n", status );
        exit( 1 );
    }
    status = rtems_task_delete( RTEMS_SELF );    /* should not return */
    printf( "rtems_task_delete returned with status of %d.\n", status );
    exit( 1 );
}

#define directive_failed(COND,MSG) do{if(COND){puts(MSG);exit(-1);}}while(0)
rtems_task Init(
  rtems_task_argument ignored
)
{
  rtems_id           task_id;
  rtems_status_code  status;

  printf("in rtems task init\n");

  puts( "INIT - rtems_task_create - creating task 1" );
  status = rtems_task_create(
    rtems_build_name( 'T', 'A', '1', ' ' ),
    1,
    RTEMS_MINIMUM_STACK_SIZE,
    RTEMS_DEFAULT_MODES,
    RTEMS_DEFAULT_ATTRIBUTES,
    &task_id
  );

  directive_failed( status, "rtems_task_create of TA1" );
  puts( "INIT - rtems_task_start - TA1 " );
  status = rtems_task_start( task_id, Periodic_task, 0 );
  directive_failed( status, "rtems_task_start of TA1" );
  while ( !partial_loop ) {
    /* main sleeps for 200 secs before checking to see if partial_loop is set */
    status = rtems_task_wake_after( 200*rtems_clock_get_ticks_per_second() );
    directive_failed( status, "rtems_task_wake_after" );
  }

  rtems_cpu_usage_reset();

  status = rtems_task_wake_after( rtems_clock_get_ticks_per_second() );
  directive_failed( status, "rtems_task_wake_after" );


  exit( 0 );
}


