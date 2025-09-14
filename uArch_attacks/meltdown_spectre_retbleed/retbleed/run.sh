#!/bin/bash

TIMELIMIT="1"
# echo $TIMELIMIT
FOLDER="assignment-3/retbleed/"
CPPATH=/data/hwsec-students/"$USER"/"$FOLDER"
# echo "$CPPATH""$1"

mkdir -p $CPPATH
cp *.c $CPPATH
cp *.h $CPPATH
cp Makefile $CPPATH
cd $CPPATH
make clean
make
srun -t $TIMELIMIT --pty "$CPPATH"retbleed #--architectural
