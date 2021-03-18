#!/bin/bash

for i in BarnesHutTest EvenOddTest FibTest FlatArrayTest ImpForTest MandelbrotTest MatrixTest Md5Test PeekTest PsdesTest TailFibTest TakTest ; do
   for s in $(seq 1 5) ; do 
      M=$(./${i}-mlton | cut -d \  -f 2)
      R=$(./${i}-rtmlton | cut -d \  -f 2)
      echo $s,$i,$M,$R
   done
done

