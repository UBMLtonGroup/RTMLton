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

