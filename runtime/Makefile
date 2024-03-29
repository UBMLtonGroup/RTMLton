## Copyright (C) 2010-2013 Matthew Fluet.
 # Copyright (C) 1999-2009 Henry Cejtin, Matthew Fluet, Suresh
 #    Jagannathan, and Stephen Weeks.
 # Copyright (C) 1997-2000 NEC Research Institute.
 #
 # MLton is released under a BSD-style license.
 # See the file MLton-LICENSE for details.
 ##

PATH := ../bin:$(shell echo $$PATH)

TARGET := self

ifeq ($(TARGET), self)
CC := gcc -std=c11 
#-std=gnu99
AR := ar rc
RANLIB := ranlib
else
CC := $(TARGET)-gcc -std=gnu99
AR := $(TARGET)-ar rc
RANLIB := $(TARGET)-ranlib
endif

TARGET_ARCH := $(shell ../bin/host-arch)
TARGET_OS := $(shell ../bin/host-os)
GCC_MAJOR_VERSION :=						\
	$(shell $(CC) -v 2>&1 | grep 'gcc version' | 		\
		sed 's/.*gcc version \([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\1/')
GCC_MINOR_VERSION :=						\
	$(shell $(CC) -v 2>&1 | grep 'gcc version' | 		\
		sed 's/.*gcc version \([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\2/')
GCC_VERSION := $(GCC_MAJOR_VERSION).$(GCC_MINOR_VERSION)

EXE :=

# These flags can be overridden by the user
CPPFLAGS :=
CFLAGS := -D_GNU_SOURCE -D_USE_GNU  $(CFLAGS)

WARNXCFLAGS :=
WARNXCFLAGS += -Wall -Werror
ifeq ($(findstring $(GCC_MAJOR_VERSION), 3),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -W
endif
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Wextra
endif
WARNXCFLAGS += -Wformat=2
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Wswitch-default -Wswitch-enum
endif
WARNXCFLAGS += -Wuninitialized
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Winit-self
endif
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Wstrict-aliasing=2
endif
WARNXCFLAGS += -Wfloat-equal
WARNXCFLAGS += -Wundef
WARNXCFLAGS += -Wshadow
WARNXCFLAGS += -Wpointer-arith
WARNXCFLAGS += -Wbad-function-cast -Wcast-qual
WARNXCFLAGS += -Wwrite-strings
# WARNXCFLAGS += -Wconversion
WARNXCFLAGS += -Waggregate-return
WARNXCFLAGS += -Wstrict-prototypes
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Wold-style-definition
endif
WARNXCFLAGS += -Wmissing-prototypes -Wmissing-declarations
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
WARNXCFLAGS += -Wmissing-field-initializers
endif
WARNXCFLAGS += -Wmissing-noreturn
WARNXCFLAGS += -Wmissing-format-attribute
# WARNXCFLAGS += -Wpacked
# WARNXCFLAGS += -Wpadded
WARNXCFLAGS += -Wredundant-decls
WARNXCFLAGS += -Wnested-externs
# WARNXCFLAGS += -Wunreachable-code

XCFLAGS := -fno-common $(WARNXCFLAGS)
OPTXCFLAGS := -Wdisabled-optimization -O2 -fomit-frame-pointer
DEBUGXCFLAGS := -DASSERT=1 -Wno-uninitialized -O0 -g -DDEBUG=1
PICXCFLAGS :=

# Win32&64 don't use PIC code, all other platforms do
ifeq ($(findstring $(TARGET_OS), mingw cygwin),)
PICXCFLAGS += -fPIC
endif

# Make mlton library symbols private (win32&64 use another technique)
ifeq ($(findstring $(GCC_MAJOR_VERSION), 4),$(GCC_MAJOR_VERSION))
ifeq ($(findstring $(TARGET_OS), mingw cygwin),)
XCFLAGS += -fvisibility=hidden
endif
endif

ifeq ($(TARGET_ARCH), alpha)
XCFLAGS += -mieee -mbwx -mtune=ev6 -mfp-rounding-mode=d
endif

ifeq ($(TARGET_ARCH), amd64)
    XCFLAGS += -m64
endif

ifeq ($(findstring $(TARGET_ARCH), hppa ia64 powerpc sparc),$(TARGET_ARCH))
ifeq (4.2, $(firstword $(sort $(GCC_VERSION) 4.2)))
# GMP headers contain C99 inline functions which generate warnings
# with a suggestion to use this flag to disable the warnings.
XCFLAGS += -fgnu89-inline
endif
endif

ifeq ($(TARGET_ARCH), ia64)
XCFLAGS += -mtune=itanium2
ifeq ($(TARGET_OS), hpux)
XCFLAGS += -mlp64
endif
endif

ifeq ($(TARGET_OS)-$(TARGET_ARCH), aix-powerpc64)
XCFLAGS += -maix64
AR := ar -X 64 rc
endif

ifeq ($(TARGET_ARCH), sparc)
XCFLAGS += -m32 -mcpu=v8 -Wa,-xarch=v8plusa
endif

ifeq ($(TARGET_ARCH), x86)
XCFLAGS += -m32
ifeq (3, $(firstword $(sort $(GCC_MAJOR_VERSION) 3)))
OPTXCFLAGS += -falign-loops=2 -falign-jumps=2 -falign-functions=5
else
OPTXCFLAGS += -malign-loops=2 -malign-jumps=2 -malign-functions=5
endif
endif

ifeq ($(TARGET_OS), cygwin)
EXE := .exe
endif

ifeq ($(TARGET_OS), darwin)
XCFLAGS += -I/usr/local/include -I/sw/include -I/opt/local/include
endif

ifeq ($(TARGET_OS), freebsd)
XCFLAGS += -I/usr/local/include
endif

ifeq ($(TARGET_OS), mingw)
EXE := .exe
# GCC doesn't recognize the %I64 format specifier which means %ll on windows
XCFLAGS += -Wno-format -Wno-missing-format-attribute
endif

ifeq ($(TARGET_OS), netbsd)
XCFLAGS += -I/usr/pkg/include
endif

ifeq ($(TARGET_OS), openbsd)
XCFLAGS += -I/usr/local/include
endif

ifeq ($(TARGET_OS), solaris)
XCFLAGS += -funroll-all-loops
endif


XCFLAGS += -I. -Iplatform
OPTCFLAGS := $(CFLAGS) $(CPPFLAGS) $(XCFLAGS) $(OPTXCFLAGS)
DEBUGCFLAGS := $(CFLAGS) $(CPPFLAGS) $(XCFLAGS) $(DEBUGXCFLAGS)
PICCFLAGS := $(OPTCFLAGS) $(PICXCFLAGS)

ALL := libgdtoa.a libgdtoa-gdb.a libgdtoa-pic.a \
       libmlton.a libmlton-gdb.a libmlton-pic.a
ALL += gen/c-types.sml gen/basis-ffi.sml gen/sizes

all: $(ALL)


### util ###

UTILHFILES :=							\
	util.h							\
	$(shell find util -type f | grep '\.h$$')
UTILCFILES :=							\
	$(shell find util -type f | grep '\.c$$')

util-pic.o: util.c $(UTILCFILES) cenv.h $(UTILHFILES)
	$(CC) $(PICCFLAGS) -c -o $@ $<
util-gdb.o: util.c $(UTILCFILES) cenv.h $(UTILHFILES)
	$(CC) $(DEBUGCFLAGS) -c -o $@ $<
util.o:     util.c $(UTILCFILES) cenv.h $(UTILHFILES)
	$(CC) $(OPTCFLAGS) -c -o $@ $<

### c-types.h  ml-types.h  gen/c-types.sml ###

c-types.h: gen/c-types.h
	cp $< $@
ml-types.h: gen/ml-types.h
	cp $< $@
gen/c-types.h gen/c-types.sml gen/ml-types.h: gen/gen-types.stamp
	@touch $@
gen/gen-types.stamp: gen/gen-types.c util.h util.o
	$(CC) $(OPTCFLAGS) -o gen/gen-types gen/gen-types.c util.o
	rm -f gen/c-types.h gen/c-types.sml gen/ml-types.h gen/gen-types.stamp
	cd gen && ./gen-types
	rm -f gen/gen-types$(EXE) gen/gen-types
	touch $@

### basis-ffi.h  gen/basis-ffi.sml ###

basis-ffi.h: gen/basis-ffi.h
	cp $< $@
gen/basis-ffi.h gen/basis-ffi.sml: gen/gen-basis-ffi.stamp
	@touch $@
gen/gen-basis-ffi.stamp: gen/gen-basis-ffi.sml gen/basis-ffi.def
	mlton -output gen/gen-basis-ffi gen/gen-basis-ffi.sml
	rm -f gen/basis-ffi.h gen/basis-ffi.sml gen/gen-basis-ffi.stamp
	cd gen && ./gen-basis-ffi
	rm -f gen/gen-basis-ffi
	touch $@

### libgdtoa ###

GDTOAHFILES := arith.h gdtoa.h
GDTOAHFILES := $(patsubst %,gdtoa/%,$(GDTOAHFILES))
GDTOACFILES :=									\
	dmisc.c     g_ddfmt.c   g_ffmt_p.c  gdtoa.c     misc.c      strtoIf.c   strtodI.c   strtopdd.c  strtord.c   sum.c \
	dtoa.c      g_ddfmt_p.c g_xLfmt.c   gethex.c    smisc.c     strtoIg.c   strtodg.c   strtopf.c   strtordd.c  ulp.c \
	g_Qfmt.c    g_dfmt.c    g_xLfmt_p.c gmisc.c     strtoIQ.c   strtoIx.c   strtof.c    strtopx.c   strtorf.c \
	g_Qfmt_p.c  g_dfmt_p.c  g_xfmt.c    hd_init.c   strtoId.c   strtoIxL.c  strtopQ.c   strtopxL.c  strtorx.c \
	g__fmt.c    g_ffmt.c    g_xfmt_p.c  hexnan.c    strtoIdd.c  strtod.c    strtopd.c   strtorQ.c   strtorxL.c
GDTOACFILES := $(patsubst %,gdtoa/%,$(GDTOACFILES))

GDTOA_OBJS       := $(patsubst %.c,%.o,$(GDTOACFILES))
GDTOA_DEBUG_OBJS := $(patsubst %.c,%-gdb.o,$(GDTOACFILES))
GDTOA_PIC_OBJS   := $(patsubst %.c,%-pic.o,$(GDTOACFILES))

gdtoa/README: gdtoa.tgz gdtoa.may_alias-unions.patch gdtoa.rename-public-fns.patch gdtoa.hide-private-fns.patch gdtoa.hide-public-fns.patch
	gzip -dc gdtoa.tgz | tar xf -
	patch -s -p0 <gdtoa.may_alias-unions.patch
	patch -s -p0 <gdtoa.rename-public-fns.patch
	patch -s -p0 <gdtoa.hide-private-fns.patch
	patch -s -p0 <gdtoa.hide-public-fns.patch
	@touch $@

gdtoa/gdtoa.h $(GDTOACFILES): gdtoa/README
	@touch $@

gdtoa/arithchk.c: gdtoa/README
	@touch $@

gdtoa/arithchk.out: gdtoa/arithchk.c
	cd gdtoa && $(CC) $(OPTCFLAGS) -w -O1 -o arithchk.out arithchk.c

gdtoa/arith.h: gdtoa/arithchk.out
	cd gdtoa && ./arithchk.out >arith.h

gdtoa/qnan.c: gdtoa/README
	@touch $@

gdtoa/qnan.out: gdtoa/arith.h gdtoa/qnan.c
	cd gdtoa && $(CC) $(OPTCFLAGS) -w -O1 -o qnan.out qnan.c

gdtoa/gd_qnan.h: gdtoa/qnan.out
	cd gdtoa && ./qnan.out >gd_qnan.h

gdtoa/%-pic.o: gdtoa/%.c gdtoa/arith.h gdtoa/gd_qnan.h
	$(CC) $(PICCFLAGS) -w -DINFNAN_CHECK -c -o $@ $<
gdtoa/%-gdb.o: gdtoa/%.c gdtoa/arith.h gdtoa/gd_qnan.h
	$(CC) $(DEBUGCFLAGS) -w -DINFNAN_CHECK -c -o $@ $<
gdtoa/%.o:     gdtoa/%.c gdtoa/arith.h gdtoa/gd_qnan.h
	$(CC) $(OPTCFLAGS) -w -DINFNAN_CHECK -c -o $@ $<

libgdtoa.a:     $(GDTOA_OBJS)
libgdtoa-gdb.a: $(GDTOA_DEBUG_OBJS)
libgdtoa-pic.a: $(GDTOA_PIC_OBJS)

### libmlton ###

PLATFORMHFILES :=						\
	platform.h						\
	$(shell find platform -type f | grep '\.h$$')
PLATFORMCFILES :=						\
	$(shell find platform -type f | grep '\.c$$')

GCHFILES :=							\
	gc.h							\
	$(shell find gc -type f | grep '\.h$$')
GCCFILES :=							\
	$(shell find gc -type f | grep '\.c$$')

BASISCFILES :=							\
	$(shell find basis -type f | grep '\.c$$')

HFILES :=							\
	cenv.h							\
	$(UTILHFILES)						\
	ml-types.h						\
	c-types.h						\
	basis-ffi.h						\
	$(PLATFORMHFILES)					\
	$(GCHFILES)

MLTON_OBJS := 							\
	util.o							\
	platform.o						\
	platform/$(TARGET_OS).o					\
	gc.o
MLTON_OBJS += $(foreach f, $(basename $(BASISCFILES)), $(f).o)
MLTON_DEBUG_OBJS := $(patsubst %.o,%-gdb.o,$(MLTON_OBJS))
MLTON_PIC_OBJS   := $(patsubst %.o,%-pic.o,$(MLTON_OBJS))

platform/$(TARGET_OS)-pic.o: $(PLATFORMCFILES)
platform/$(TARGET_OS)-gdb.o: $(PLATFORMCFILES)
platform/$(TARGET_OS).o:     $(PLATFORMCFILES)

gc-pic.o: $(GCCFILES)
gc-gdb.o: $(GCCFILES)
gc.o:     $(GCCFILES)

gc.c_XCFLAGS := -Wno-unreachable-code

basis/Real/Math-pic.o:  basis/Real/Math-fns.h
basis/Real/Math-gdb.o:  basis/Real/Math-fns.h
basis/Real/Math.o:      basis/Real/Math-fns.h
basis/Real/Real-pic.o:  basis/Real/Real-ops.h
basis/Real/Real-gdb.o:  basis/Real/Real-ops.h
basis/Real/Real.o:      basis/Real/Real-ops.h
basis/Real/Real.c_XCFLAGS := -Wno-float-equal
basis/Real/gdtoa-pic.o: $(GDTOAHFILES)
basis/Real/gdtoa-gdb.o: $(GDTOAHFILES)
basis/Real/gdtoa.o:     $(GDTOAHFILES)
basis/Real/strto-pic.o: $(GDTOAHFILES)
basis/Real/strto-gdb.o: $(GDTOAHFILES)
basis/Real/strto.o:     $(GDTOAHFILES)
basis/System/Date.c_XCFLAGS := -Wno-format-nonliteral
basis/Word/Word-pic.o:  basis/Word/Word-consts.h basis/Word/Word-ops.h basis/Word/Word-check.h
basis/Word/Word-gdb.o:  basis/Word/Word-consts.h basis/Word/Word-ops.h basis/Word/Word-check.h
basis/Word/Word.o:      basis/Word/Word-consts.h basis/Word/Word-ops.h basis/Word/Word-check.h
basis/coerce-pic.o:     basis/coerce.h
basis/coerce-gdb.o:     basis/coerce.h
basis/coerce.o:         basis/coerce.h
basis/cpointer-pic.o:   basis/cpointer.h
basis/cpointer-gdb.o:   basis/cpointer.h
basis/cpointer.o:       basis/cpointer.h

%-pic.o: %.c $(HFILES)
	$(CC) $(PICCFLAGS) $($(<)_XCFLAGS) -c -o $@ $<
%-gdb.o: %.c $(HFILES)
	$(CC) $(DEBUGCFLAGS) $($(<)_XCFLAGS) -c -o $@ $<
%.o: %.c $(HFILES)
	$(CC) $(OPTCFLAGS) $($(<)_XCFLAGS) -c -o $@ $<

libmlton.a: $(MLTON_OBJS)
libmlton-gdb.a: $(MLTON_DEBUG_OBJS)
libmlton-pic.a: $(MLTON_PIC_OBJS)

### gen/sizes ###

gen/sizes: gen/gen-sizes.stamp
	@touch $@
gen/gen-sizes.stamp: gen/gen-sizes.c libmlton.a $(HFILES)
	$(CC) $(OPTCFLAGS) -o gen/gen-sizes gen/gen-sizes.c -L. -lmlton
	rm -f gen/sizes
	cd gen && ./gen-sizes
	rm -f gen/gen-sizes$(EXE) gen/gen-sizes
	touch $@


######


%.a:
	rm -f $@
	$(AR) $@ $^
	$(RANLIB) $@



.PHONY: flags
flags:
	echo TARGET = $(TARGET)
	echo TARGET_ARCH = $(TARGET_ARCH)
	echo TARGET_OS = $(TARGET_OS)
	echo GCC_MAJOR_VERSION = $(GCC_MAJOR_VERSION)
	echo GCC_MINOR_VERSION = $(GCC_MINOR_VERSION)
	echo GCC_VERSION = $(GCC_VERSION)
	echo EXE = $(EXE)
	echo OPTXCFLAGS = $(OPTXCFLAGS)
	echo DEBUGXCFLAGS = $(DEBUGXCFLAGS)
	echo PICXCFLAGS = $(PICXCFLAGS)


.PHONY: clean
clean:
	../bin/clean

.PHONY: check
check: libmlton-gdb.a
	# if you want to use gdb on this exec, first export CK_FORK=no
	checkmk umarray-allocate-test.check > umarray-allocate-test.c
	gcc -g -O0 -I. -Iplatform  -pthread  -o umarray-allocate-test umarray-allocate-test.c -lcheck  -lrt -lm -lpthread -lsubunit ./libmlton-gdb.a ./libgdtoa-gdb.a 
	CK_VERBOSITY=verbose ./umarray-allocate-test

splitheader: splitheader.c
	gcc -g -O0 -I. -Iplatform  -pthread  -o splitheader splitheader.c  -lrt  -lpthread -L . -lmlton-gdb ./libgdtoa-gdb.a
