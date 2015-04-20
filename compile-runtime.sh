#!/bin/bash
TARGET=x86_64-pc-linux-gnu
arch_flags="TARGET=$TARGET TARGET_OS=linux TARGET_ARCH=x86"
rm -rf build/lib/targets/$TARGET/  &&
cd runtime && make clean && cd .. &&
mkdir build/lib/targets/$TARGET/ &&
mkdir build/lib/targets/$TARGET/include &&
mkdir build/lib/targets/$TARGET/sml &&
make  $arch_flags runtime &&
make $arch_flags constants &&
make mlbpathmap

