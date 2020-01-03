
util/annotate.py ub/justprint.2.c > ub/justprint.2a.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/targets/self/include \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m32 -fno-strength-reduce -fschedule-insns -fschedule-insns2 \
          -falign-functions=5 -falign-jumps=2 -falign-loops=2 -O0 -o \
          /tmp/fileOzNFRT.o ub/justprint.2a.c

util/annotate.py ub/justprint.1.c > ub/justprint.1a.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/targets/self/include \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m32 -fno-strength-reduce -fschedule-insns -fschedule-insns2 \
          -falign-functions=5 -falign-jumps=2 -falign-loops=2 -O0 -o \
          /tmp/filekMcgys.o ub/justprint.1a.c
      gcc -std=gnu99 -c -g -DASSERT=1 \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/targets/self/include \
          -I/media/sf_cse/CSE/B/RTMLton/build/lib/include -O1 -fno-common \
          -D_GNU_SOURCE -D__USE_GNU -fno-strict-aliasing -fomit-frame-pointer \
          -w -m32 -fno-strength-reduce -fschedule-insns -fschedule-insns2 \
          -falign-functions=5 -falign-jumps=2 -falign-loops=2 -O0 -o \
          /tmp/fileR1ClcO.o ub/justprint.0.c
      gcc -o ub/justprint -g /tmp/fileOzNFRT.o /tmp/filekMcgys.o \
          /tmp/fileR1ClcO.o \
          -L/media/sf_cse/CSE/B/RTMLton/build/lib/targets/self -lmlton-gdb \
          -lgdtoa-gdb -lm -lgmp -lpthread -lrt -m32
