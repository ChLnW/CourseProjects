#!/bin/bash

cp -r * /data/hwsec-students/"$USER"/assignment-3/
cd /data/hwsec-students/"$USER"/assignment-3/
make clean
make all
# srun -t 20:00 --pty /data/hwsec-students/"$USER"/assignment-3/meltdown_segv
srun -t 20:00 --pty /data/hwsec-students/"$USER"/assignment-3/grade.sh -v meltdown_spectre
# make all && objdump -d -M intel meltdown_segv | grep -A 25 "<meltdown_sighandler>"
