/*
 * Hello world example
 */
#include <rtems.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include "/home/jcmurphy/quick-start/rtems/6/i386-rtems6/include/stdio.h"

int stderr = 0;
int mainX(int, char **);

rtems_task Init(
  rtems_task_argument ignored
)
{
  int argc = 1;
  char* argv[1];
  argv[0] = "justprint";

  printf("in rtems task init\n");
  mainX(argc, argv);
  exit( 0 );
}
