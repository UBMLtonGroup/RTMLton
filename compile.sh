#!/bin/bash
rm -rf build

export CFLAGS="-Wunused-but-set-variable -DSTACK_GC_SANITY"
arch_flags="-j 8"
make $arch_flags dirs     &&
make $arch_flags runtime  &&
make $arch_flags compiler &&
make $arch_flags basis-no-check &&
make $arch_flags script &&
make $arch_flags mlbpathmap &&
make $arch_flags constants 

cat <<EOF
Try:
 ./build/bin/mlton  -codegen c -keep g -debug true -expert true -verbose 3 -keep ssa -keep ssa2 -keep rssa -keep machine -cc-opt -O0 ub/justprint.sml
 ./justprint

 adjust STACKLET_DEBUG in include/c-chunk.h and
 cp include/c-chunk.h build/lib/include/
 to debug stacklets
EOF


exit 0

TARGET=x86_64-pc-linux-gnu
arch_flags="TARGET=$TARGET TARGET_OS=linux TARGET_ARCH=x86"
make  dirs     &&
cd runtime && make clean && cd .. &&
make  runtime  &&
make  compiler &&
make  basis-no-check &&
make  script &&
make  mlbpathmap &&
make  constants  &&
cd runtime && make clean && cd .. &&
mkdir build/lib/targets/$TARGET/ &&
mkdir build/lib/targets/$TARGET/include &&
mkdir build/lib/targets/$TARGET/sml &&
make  $arch_flags runtime &&
make constants &&
make mlbpathmap

