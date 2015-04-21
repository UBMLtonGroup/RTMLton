#!/bin/bash
rm -rf build

arch_flags=""
make $arch_flags dirs     &&
make $arch_flags runtime  &&
make $arch_flags compiler &&
make $arch_flags basis-no-check &&
make $arch_flags script &&
make $arch_flags mlbpathmap &&
make $arch_flags constants 


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

