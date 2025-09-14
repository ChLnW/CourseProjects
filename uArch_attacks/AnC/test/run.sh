#!/bin/bash

TIMELIMIT="1"
# echo $TIMELIMIT

mv $1.notc $1.c
gcc $1.c -o $1
cp $1 /data/hwsec-students/"$USER"/
srun -t $TIMELIMIT --pty /data/hwsec-students/"$USER"/$1
# srun -t 1 --pty /data/hwsec-students/"$USER"/$1
mv $1.c $1.notc 