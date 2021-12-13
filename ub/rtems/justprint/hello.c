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
  printf("in rtems task init\n");
  mainX();
  exit( 0 );
}
