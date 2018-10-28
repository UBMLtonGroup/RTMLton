#!/bin/sh -x


      gcc -std=gnu99 -c -g -DASSERT=1 -I/usr/lib/mlton/targets/self/include \
          -I/usr/lib/mlton/include -O1 -fno-common -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/filexri3DY.o test-vanilla.1.c
      gcc -std=gnu99 -c -g -DASSERT=1 -I/usr/lib/mlton/targets/self/include \
          -I/usr/lib/mlton/include -O1 -fno-common -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/file9PIEte.o test-vanilla.0.c
      gcc -o test-vanilla -g /tmp/filexri3DY.o /tmp/file9PIEte.o \
          -L/usr/lib/mlton/targets/self -lmlton-gdb -lgdtoa-gdb -lm -lgmp \
          -m64 -Wl,-znoexecstack

