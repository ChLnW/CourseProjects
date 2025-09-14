#!/bin/bash

TIMELIMIT="2"
# echo $TIMELIMIT

# cp *.c /data/hwsec-students/"$USER"/assignment-2/
# cp *.h /data/hwsec-students/"$USER"/assignment-2/
# cp *.py /data/hwsec-students/"$USER"/assignment-2/
# cp Makefile /data/hwsec-students/"$USER"/assignment-2/
cp -r * /data/hwsec-students/"$USER"/assignment-2/
cd /data/hwsec-students/"$USER"/assignment-2/
make clean
make anc
srun -t $TIMELIMIT --pty /data/hwsec-students/"$USER"/assignment-2/anc $1
