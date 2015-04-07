
# Overview

1. given hw.sml 
2. mlton -keep c -stop c hw.sml -> results in c code
3. rtems-gcc (c code from step 2) -> results in rtems binary

# Libraries

In order to do step 3, we need to have any libraries that
mlton links the emitted c-code against built for the RTEMS
target platform. 

For a simple 'hello world' app, mlton links with 

```-lmlton -lgdtoa -lm -lgmp```

# Edits

```mlton/include/cenv.h``` must be modified with the
appropriate platform/arch includes (otherwise you get an unknown
platform/arch #error)

For this project, arch is sparc

