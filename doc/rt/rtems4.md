# Working on Cross-compiling MLton to RTEMS (Sparc)
## What to Expect from this Post
Currently the MLton runtime compiles against Sparc RTEMS (Leon 3
BSP). The MLton is capable of recognizing RTEMS and Sparc in -target
flag. But the compiled executable *does not work*.

## Steps
### Get MLton Source Code

    git clone git@github.com:UBMLtonGroup/MLton.git

### Install Binary MLton Compiler
MLton needs an ML compiler to compile. This is somewhat a chicken-egg
dilema. You'll need to install a binary verion of MLton first. It's
not a big deal in most big Linux distros. I'm using Gentoo Linux, so
the following command works for me:

    emerge mlton

### Get RTEMS Ready
#### Install RTEMS Compilation Toolchain
You can follow the manual on (RTEMS Source
Builder)[https://ftp.rtems.org/pub/rtems/people/chrisj/source-builder/source-builder.html]
to get a working RTEMS compilation toolchain. The following part
assumes programs in the toolchain are in your PATH (e.g. you should be
able to directly call sparc-rtems4.10-gcc in your terminal).

#### Install RTEMS
Follow the (quick
start)[https://devel.rtems.org/wiki/TBR/UserManual/Quick_Start] to get
RTEMS ready.

#### Grab a Simulator
The target BSP is Leon3, so you'll want TSIM-Leon3. An evaluated
verion is (available)[http://www.gaisler.com/index.php/downloads/simulators].

### Compile MLton Runtime
The following compilation process is an illustration of trial and
error. It involves a lot of human involvement. Automation is possible
but I have to first make sure everything is working properly.

#### Move MLton Runtime to a Different Directory
This is *highly recommended* as to build the runtime against RTEMS,
you'll need to tweak the source code in various ways, which may
interfere with following compilation process. I'm just lazy so I copy
the whole repository to a different location.

    cp MLton -r MLton_cross
    cd MLton_cross

### Build GMP Library
MLton depends on GMP. Fortunately, GMP is not hard to port. The
following steps should get you started:

1. Get GMP source
2. Configure with --host=sparc-rtems4.10 --prefix=<where you want to
   install it>
3. Type make and make install

#### Cross-Compiling MLton Runtime
To specify target, you can supply TARGET, TARGET\_OS, TARGET\_ARCH to
make. To compile with Leon3 BSP, you also add some extra flags. So the
compilation command would become something like this:

    make TARGET=sparc-rtems4.10 TARGET_OS=rtems TARGET_ARCH=sparc \
        CFLAGS="-pipe -B/mnt/rtems/rtems/development/rtems/rtems-leon3/sparc-rtems4.10/leon3/lib \
        -specs bsp_specs -qrtems -g -Wall -O2 -mcpu=cypress \
        -msoft-float -B/mnt/rtems/gmp" COMPILE_FAST=yes runtime

Here, my GMP is in /mnt/rtems/gmp. You'll see the compiler rolling for
a few seconds and then, it stops:

    /bin/sh: ./arithchk.out: cannot execute binary file
    Makefile:261: recipe for target 'gdtoa/arith.h' failed

The compilation of MLton will run some newly compiled small programs
to detect a few architecture specific configurations. Since you are
compiling for Sparc, it's not possible to run on Linux amd64.

You can use TSIM to run the program. But there is a catch, for
whatever reason, programs with "return 0" in main won't quit in
TSIM. You can either wait and kill it or change "return 0" to
"exit(0)" to force the program to quit.

    tsim-leon3 runtime/gdtoa/arithchk.out
    tsim> run
    starting at 0x40000000
    #define IEEE_MC68k
    #define Arith_Kind_ASL 2
    #define Double_Align

These 3 #define lines are what meant to be put in
runtime/gdtoa/arith.h. You can copy and paste them to the file and
continue to make. 1s later, you'll run into a similar problem.

    /bin/sh: ./qnan.out: cannot execute binary file
    Makefile:270: recipe for target 'gdtoa/gd_qnan.h' failed

Ah! The same deal! Paste the following #define lines to runtime/gdtoa/gd_qnan.h

    tsim-leon3 runtime/gdtoa/qnan.out
    tsim> run
    starting at 0x40000000
    #define f_QNAN 0x7f900000
    #define d_QNAN0 0x7ff80000
    #define d_QNAN1 0x0

The POSIX libraries shipped with the toolchain does not have
fenv.h. So you'll need to define NO\_FENV\_H somewhere to avoid
trouble. For me, I just put it in gd_qnan.h:

    /* gd_qnan.h */
    #define f_QNAN 0x7f900000
    #define d_QNAN0 0x7ff80000
    #define d_QNAN1 0x0
    #define NO_FENV_H

We keep going. This time the compilation keeps going for a while and
stops at some seemingly strange errors:

    In file included from ./util.h:12,
                     from util/die.c:8,
                     from util.c:9:
    ./cenv.h:96:2: error: #error unknown platform os
    ./cenv.h:132:5: warning: "UINTPTR_MAX" is not defined
    ./cenv.h:132:20: warning: "UINT32_MAX" is not defined
    ./cenv.h:134:7: warning: "UINTPTR_MAX" is not defined
    ./cenv.h:134:22: warning: "UINT64_MAX" is not defined
    In file included from ./util.h:12,
                     from util/die.c:8,
                     from util.c:9:
    ./cenv.h:145: error: 'uintptr_t' undeclared here (not in a function)
    In file included from ./util.h:37,
                     from util/die.c:8,
                     from util.c:9:
    ./util/safe.h: In function 'calloc_safe':
    ./util/safe.h:14: error: expected ')' before 'PRIuMAX'
    ./util/safe.h:15: warning: spurious trailing '%' in format
    ./util/safe.h: In function 'fread_safe':
    ./util/safe.h:13: error: expected ')' before 'PRIxPTR'
    ./util/safe.h:54: warning: conversion lacks type at end of format
    ./util/safe.h: In function 'fseek_safe':
    ./util/safe.h:63: error: expected ')' before 'PRIuMAX'
    ./util/safe.h:64: warning: spurious trailing '%' in format

This is because we have not told MLton some platform specific
knowledge yet. Open runtime/cenv.h and add following lines:

    #include "platform/openbsd.h"
    #elif (defined (__sun__))
    #include "platform/solaris.h"
    /* The above 3 lines are context information */
    #elif (defined (__rtems__))
    #include "platform/rtems_env.h"

Then copy the rtems_env.h and rtems.c in this repository to
runtime/platform #folder. *I suspect these 2 files truly need to be
refined. These 2 files should be the primary factor that contributes
to malfunctioning of the compiled executable* Then keep going and
you'll run into the following problem:

    /bin/sh: ./gen-types: cannot execute binary file
    Makefile:208: recipe for target 'gen/gen-types.stamp' failed

The same deal? You wonder. But this time, you'll need to play
careful. Copy the files in the "gen" folder of this repository to
runtime/gen folder and don't forget to *touch a stamp*.

    cp ../../MLT/RTMLton/runtime/gen/* runtime/gen/
    touch runtime/gen/gen-types.stamp

The compilation will continue and it stops at:

    cd gen && ./gen-sizes
    /bin/sh: ./gen-sizes: cannot execute binary file
    Makefile:368: recipe for target 'gen/gen-sizes.stamp' failed

Use tsim-leon3 to execute runtime/gen/gen-sizes and paste the output
to runtime/gen/sizes, then touch a timestamp. Old trick! But
wait... the gen-size.c uses fprintf to print to file, which is not
supported by tsim-leon3. You'll need some tweaks! I change the file
descriptor to stdout:

      sizesFd = stdout; //fopen_safe ("sizes", "w");

then recompiles and executes it.

    tsim-leon3 runtime/gen/gen-sizes
    tsim> run
    starting at 0x40000000
    cint = 4
    cpointer = 4
    cptrdiff = 4
    csize = 4
    header = 4
    mplimb = 4
    objptr = 4
    seqIndex = 4
    ...
    touch runtime/gen/gen-sizes.stamp

and we are done!

### Compiling MLton
Now we move back to the original verion of MLton. You'll tweak a few
places to get it to know "Sparc" and "RTEMS". Please refer to the
(official document)[http://mlton.org/PortingMLton] for more
information.

    grep "Rtems" -nir basis-library
    basis-library/mlton/platform.sig:32:                       | Linux | MinGW | NetBSD | OpenBSD | Solaris | Rtems
    basis-library/mlton/platform.sml:80:                (Rtems,   "Rtems")]
    basis-library/primitive/prim-mlton.sml:214:             | Rtems
    basis-library/primitive/prim-mlton.sml:229:                | "rtems" => Rtems
    basis-library/sml-nj/sml-nj.sml:45:                   | Rtems => UNIX

    grep rtems -inr mlton/
    mlton/main/main.fun:1136:          | Rtems => ()

Now compile MLton with a glorious "make" (I don't want to wait for the
docs to be generated):

    make all-no-docs

Then we copy all the files we created before to this brand-new MLton:

    mkdir build/lib/targets/sparc-rtems4.10
    mkdir build/lib/targets/sparc-rtems4.10/{include,sml}
    cp ../MLton_cross/runtime/*.a build/lib/targets/sparc-rtems4.10/
    cp ../MLton_cross/runtime/gen/c-types.h build/lib/targets/sparc-rtems4.10/include
    cp ../MLton_cross/runtime/gen/c-types.sml build/lib/targets/sparc-rtems4.10/sml
    cp ../MLton_cross/runtime/gen/sizes build/lib/targets/sparc-rtems4.10/

Copy the files in build/lib/targets/sparc-rtems4.10 in this repository
to build/lib/targets/sparc-rtems4.10/. Now the folder should contain:

    ls build/lib/targets/sparc-rtems4.10/
    arch       include     libgdtoa-gdb.a  libmlton.a      libmlton-pic.a  sml
    constants  libgdtoa.a  libgdtoa-pic.a  libmlton-gdb.a  os

When you type build/bin/mlton --help, you should see sparc-rtems4.10
in the -targets part.

You'll also need to copy our patched version of cenv.h,
platform/rtems_env.h to the newly build MLton path:

    cp ../MLton_cross/build/lib/include/cenv.h build/lib/include/
    cp ../MLton_cross/build/lib/include/platform/rtems_env.h build/lib/include/platform/

The last thing is to modify build/bin/mlton script to understand RTEMS
compilation flags and GMP linking options:

    cat build/bin/mlton | grep -n -C 3 -i leon
    117-        -target-cc-opt openbsd '-I/usr/local/include'            \
    118-        -target-cc-opt aix '-maix64'                             \
    119-        -target-cc-opt ia64 "$ia64hpux -mtune=itanium2"          \
    120:        -target-cc-opt rtems '-pipe -B/mnt/rtems/rtems/development/rtems/rtems-leon3/sparc-rtems4.10/leon3/lib -specs bsp_specs -qrtems -g -Wall -O2 -mcpu=cypress -msoft-float -B/mnt/rtems/gmp' \
    121-        -target-cc-opt sparc '-m32 -mcpu=v8 -Wa,-xarch=v8plusa'  \
    122-        -target-cc-opt x86                                       \
    123-                '-m32
    --
    143-        -target-link-opt openbsd '-L/usr/local/lib/'             \
    144-        -target-link-opt solaris '-lnsl -lsocket -lrt'           \
    145-        -target-link-opt x86 '-m32'                              \
    146:        -target-link-opt rtems '-pipe -B/mnt/rtems/rtems/development/rtems/rtems-leon3/sparc-rtems4.10/leon3/lib -specs bsp_specs -qrtems -g -Wall -O2 -mcpu=cypress -msoft-float -B/mnt/rtems/gmp -L/mnt/rtems/gmp/lib'            \
    147-         -profile-exclude '\$\(SML_LIB\)'                         \
    148-        "$@"

The last thing,
### Testing the Compilation
Now you can compile a hello_world program.

    build/bin/mlton hello.sml

and you'll see an executable hello being generated. Unfortunately,
when I load it to TSIM-Leon3, it does not work at the moment:

    build/bin/mlton -target sparc-rtems4.10 hello.sml
    tsim-leon3 hello
    tsim> run
    starting at 0x40000000
    Unexpected trap (43) at address 0x4005BE3C

## What's Next?
I feel I *REALLY* need to look into the rtems.c and rtems_env.h to
find out the problem. These lines are added rather ad-hoc to keep the
compilation going. It seems that a few parameters / methods
implementations are not correct at the moment.