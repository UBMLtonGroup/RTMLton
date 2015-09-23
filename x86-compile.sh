#!/bin/bash
rm -rf *.c
build/bin/mlton -codegen c -keep rssa -target x86_64-pc-linux-gnu -verbose 3 $1
#gcc -pg -g3 -O2 -m32 *.c -o eq  -Ibuild/lib/targets/x86_64-pc-linux-gnu/include -Ibuild/lib/include -lgmp -Lbuild/lib/targets/x86_64-pc-linux-gnu/ -lmlton -lgdtoa-pic -l gdtoa -lgdtoa-gdb -lmlton-pic -lm
