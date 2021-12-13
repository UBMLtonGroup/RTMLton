#!/bin/bash 

trap exit INT

for i in $(seq 0 100) ; do
   echo -n exn,$i,
   /usr/bin/time -f %e ./exn 1000 > /dev/null
   echo -n noexn,$i,
   /usr/bin/time -f %e ./noexn 1000 > /dev/null
done


