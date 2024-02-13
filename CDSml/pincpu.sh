#!/bin/bash -x
#
# use this to pin deadline scheduled tasks
# to cpu0 in rtlinux

mkdir -p /dev/cpuset/cpu0
mount -t cgroup -o cpuset cpuset /dev/cpuset

echo 0 > /dev/cpuset/cpu0/cpuset.cpus
echo 0 > /dev/cpuset/cpu0/cpuset.mems
echo 1 > /dev/cpuset/cpuset.cpu_exclusive
echo 0 > /dev/cpuset/cpuset.sched_load_balance
echo 1 > /dev/cpuset/cpu0/cpuset.cpu_exclusive
echo 1 > /dev/cpuset/cpu0/cpuset.mem_exclusive

echo $$ > /dev/cpuset/cpu0/tasks
echo ./runCD @MLton lower-bound 40 upper-bound 200 hpercent 0.9 rtthreads true max-heap $3 packingstage1 $1 packingstage2 $2 --
bash 

#sleep 10
#exec ./runCD @MLton lower-bound 40 upper-bound 200 hpercent 0.9 rtthreads true max-heap $3 packingstage1 $1 packingstage2 $2 --

