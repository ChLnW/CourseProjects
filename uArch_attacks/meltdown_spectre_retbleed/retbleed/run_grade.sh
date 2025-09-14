#!/bin/bash

TIMELIMIT="20"
# echo $TIMELIMIT
FOLDER="assignment-3/retbleed/"
CPPATH=/data/hwsec-students/"$USER"/"$FOLDER"
# echo "$CPPATH""$1"

make clean
mkdir -p $CPPATH
cp -r * $CPPATH
# cp *.c $CPPATH
# cp *.h $CPPATH
# cp Makefile $CPPATH
cd $CPPATH
make clean
make
# srun -t $TIMELIMIT --pty "$CPPATH"retbleed #--architectural
srun -t 20:00 --pty "$CPPAT"grade.sh
