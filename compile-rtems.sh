#!/bin/bash 


export RTEMS=/opt/rtems6
export PATH=$RTEMS/bin:$PATH

mkdir -p build/lib/targets/i386-rtems6/sml
mkdir -p build/lib/targets/i386-rtems6/include
find runtime -name \*.[oa] -ls -exec rm {} \;

export TARGET=i386-rtems6
export TARGET_OS=rtems
export TARGET_ARCH=i386
export CFLAGS="-pipe -B$RTEMS/i386-rtems6/pc386/lib  -qrtems -g -Wall -O0 -L/opt/rtems6/lib -lrtemsdefaultconfig -I/opt/rtems6/include  "
export COMPILE_FAST=yes

make_flags="-j 1 -f Makefile.rtems TARGET_OS=${TARGET_OS} TARGET=${TARGET} TARGET_ARCH=${TARGET_ARCH} " CFLAGS=$CFLAGS
echo $make_flags


#make $make_flags dirs     &&
make $make_flags runtime 
#make $make_flags compiler && make $make_flags basis-no-check && make $make_flags script && make $make_flags mlbpathmap && make $make_flags constants && make $make_flags libraries
