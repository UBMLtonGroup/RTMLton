/*
 * Hello world example
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

int mainX(void);

rtems_task Init(
  rtems_task_argument ignored
)
{
  mainX();
  exit( 0 );
}
