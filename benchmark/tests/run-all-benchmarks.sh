#!/bin/bash
FMT="%e %M %K"

trap ctrl_c INT

function ctrl_c() {
    exit 1
}


exec 3>&1 

if [ ! -e filelist.txt ] ; then
    ls -1 *.sml > filelist.txt
fi

for i in `cat filelist.txt` ; do
    cp $i doit.sml
    echo " " >> doit.sml
    base=$(basename $i .sml)
    N=$(grep $base run-all-benchmarks.cfg | cut -d , -f 2 | cut -d \) -f 1)
    echo "val _ = Main.doit $N" >> doit.sml
    echo "$i iterations $N"

    /usr/bin/time -f "$i mlton-compile $FMT" mlton -codegen c doit.sml 
    echo "$i mlton-codesize " $(stat -c "%s" doit)
    /usr/bin/time -f "$i mlton-runtime $FMT" ./doit

    /usr/bin/time -f "$i rtmlton-compile $FMT" ../../build/bin/mlton -codegen c ./doit.sml 
    echo "$i rtmlton-codesize " $(stat -c "%s" doit)
    /usr/bin/time -f "$i rtmlton-runtime $FMT" ./doit 2>&1 >&3 | tee ${i}-rtmlton-stats

    rm doit.sml
done
