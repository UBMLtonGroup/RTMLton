#!/bin/bash
rm -rf *.c
build/bin/mlton -codegen c -keep g -target x86_64-pc-linux-gnu $1
gcc *.c -o eq  -Ibuild/lib/targets/self/include -Ibuild/lib/include -lgmp -Lbuild/lib/targets/self -lmlton -lgdtoa-pic -l gdtoa -lgdtoa-gdb -lmlton-pic -lm
