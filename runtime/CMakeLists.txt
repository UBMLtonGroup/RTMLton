cmake_minimum_required (VERSION 2.4)
project (MLton-Runtime)

option(MLTON_GC_INTERNAL_TYPES "..." ON)
option(MLTON_GC_INTERNAL_FUNCS "..." ON)
add_definitions(-DMLTON_GC_INTERNAL_TYPES -DMLTON_GC_INTERNAL_FUNCS)

include_directories( . ../include gc )


set(SRC gc/um.c gc/umheap.c gc/um.h gc/umheap.h)

add_library(libmlton.a ${SRC})

