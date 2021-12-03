/*
 * Hello world example
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include "/home/jcmurphy/quick-start/rtems/6/i386-rtems6/include/stdio.h"

int mainX(void);

rtems_task Init(
  rtems_task_argument ignored
)
{
  mainX();
  exit( 0 );
}
