#!/bin/sh 

mkdir -p build/lib/targets/i386-rtems6/sml

make -f Makefile.rtems TARGET=i386-rtems6 TARGET_OS=rtems TARGET_ARCH=i386 CFLAGS="-pipe -B$HOME/quick-start/rtems/6/i386-rtems6/pc386/lib -specs /home/jcmurphy/quick-start/src/rtems/bsps/i386/pc386/start/bsp_specs -qrtems -g -Wall -O2  -I/home/jcmurphy/quick-start/src/rsb/rtems/build/gmp-6.1.0-i686-linux-gnu-1/gmp-6.1.0/ -lrtemsdefaultconfig " COMPILE_FAST=yes runtime
