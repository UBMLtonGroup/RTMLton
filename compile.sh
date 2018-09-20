#!/bin/bash
TARGET=x86-pc-linux-gnu
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

