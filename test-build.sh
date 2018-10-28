#!/bin/sh -x
 ./build/bin/mlton -codegen c -keep g -verbose 3 -debug true -keep machine -keep ssa -keep ast test.sml
