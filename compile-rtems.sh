#!/bin/bash 

export PATH=$HOME/quick-start/rtems/6/bin:$PATH

mkdir -p build/lib/targets/i386-rtems6/sml
mkdir -p build/lib/targets/i386-rtems6/include
find runtime -name \*.a -name \*.o -ls -exec rm {} \;

export TARGET=i386-rtems6
export TARGET_OS=rtems
export TARGET_ARCH=i386
export CFLAGS="-pipe -B$HOME/quick-start/rtems/6/i386-rtems6/pc386/lib -specs /home/jcmurphy/quick-start/src/rtems/bsps/i386/pc386/start/bsp_specs -qrtems -g -Wall -O2  -I/home/jcmurphy/quick-start/src/rsb/rtems/build/gmp-6.1.0-i686-linux-gnu-1/gmp-6.1.0/ -lrtemsdefaultconfig "
export COMPILE_FAST=yes

make_flags="-j 1 -f Makefile.rtems TARGET_OS=${TARGET_OS} TARGET=${TARGET} TARGET_ARCH=${TARGET_ARCH} " CFLAGS=$CFLAGS
echo $make_flags


#make -f Makefile.rtems TARGET=i386-rtems6 TARGET_OS=rtems TARGET_ARCH=i386 CFLAGS="-pipe -B$HOME/quick-start/rtems/6/i386-rtems6/pc386/lib -specs /home/jcmurphy/quick-start/src/rtems/bsps/i386/pc386/start/bsp_specs -qrtems -g -Wall -O2  -I/home/jcmurphy/quick-start/src/rsb/rtems/build/gmp-6.1.0-i686-linux-gnu-1/gmp-6.1.0/ -lrtemsdefaultconfig " COMPILE_FAST=yes runtime


#make $make_flags dirs     &&
make $make_flags runtime 
#make $make_flags compiler && make $make_flags basis-no-check && make $make_flags script && make $make_flags mlbpathmap && make $make_flags constants && make $make_flags libraries
