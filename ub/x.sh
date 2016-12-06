      gcc -std=gnu99  -g -c -I/usr/lib/mlton/targets/self/include \
          -I/usr/lib/mlton/include -O1 -fno-common -fno-strict-aliasing \
          -fomit-frame-pointer -w -m64 -o /tmp/file45KNHD.o test2.1.c
      gcc -c -g -m64 -o /tmp/file8rnteo.o test2.0.s
      gcc -g  -o test2 /tmp/file45KNHD.o /tmp/file8rnteo.o \
          -L/usr/lib/mlton/targets/self -lmlton -lgdtoa -lm -lgmp -m64 \
          -Wl,-znoexecstack
