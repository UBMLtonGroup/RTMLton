
      gcc -g -std=gnu99 -c -I/home/jcmurphy/MLton/build/lib/targets/self/include \
          -I/home/jcmurphy/MLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m64 -o /tmp/fileDkYBXQ.o test2.2.c
      gcc -g -std=gnu99 -c -I/home/jcmurphy/MLton/build/lib/targets/self/include \
          -I/home/jcmurphy/MLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m64 -o /tmp/file72LZTv.o test2.1.c
      gcc -g -std=gnu99 -c -I/home/jcmurphy/MLton/build/lib/targets/self/include \
          -I/home/jcmurphy/MLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m64 -o /tmp/file4K5iDT.o test2.0.c
      gcc -g -o test2 /tmp/fileDkYBXQ.o /tmp/file72LZTv.o /tmp/file4K5iDT.o \
          -L/home/jcmurphy/MLton/build/lib/targets/self -lmlton -lgdtoa -lm \
          -lgmp -m64 -lpthread -lrt

