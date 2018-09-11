#!/bin/sh -x
gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self/include \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/include -O1 \
          -fno-common -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/fileVvXl5Y.o test.2.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self/include \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/include -O1 \
          -fno-common -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/file9186bX.o test.1.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self/include \
          -I/media/psf/Home/git/cse/CSE/RTMLton/build/lib/include -O1 \
          -fno-common -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/file8ENuHs.o test.0.c
      gcc -o test -g /tmp/fileVvXl5Y.o /tmp/file9186bX.o /tmp/file8ENuHs.o \
          -L/media/psf/Home/git/cse/CSE/RTMLton/build/lib/targets/self \
          -lmlton-gdb -lgdtoa-gdb -lm -lgmp -m64 -lpthread -lrt


