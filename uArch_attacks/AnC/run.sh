#!/bin/bash

TIMELIMIT="1"
# echo $TIMELIMIT


cp *.c /data/hwsec-students/"$USER"/assignment-2/
cp *.h /data/hwsec-students/"$USER"/assignment-2/
cp Makefile /data/hwsec-students/"$USER"/assignment-2/
cd /data/hwsec-students/"$USER"/assignment-2/
make clean
make $1
srun -t $TIMELIMIT --pty /data/hwsec-students/"$USER"/assignment-2/$1
