#!/bin/sh -x
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self/include \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/include -O1 \
          -fno-common -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/fileuYL7pj.o test-nobasis.1.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self/include \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/include -O1 \
          -fno-common -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/filety4Zuz.o test-nobasis.0.c
      gcc -o test-nobasis -g /tmp/fileuYL7pj.o /tmp/filety4Zuz.o \
          -L/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self \
          -lmlton-gdb -lgdtoa-gdb -lm -lgmp -m64 -lpthread -lrt

