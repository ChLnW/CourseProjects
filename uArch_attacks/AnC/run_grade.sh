#!/bin/bash

cp -r * /data/hwsec-students/"$USER"/assignment-2/
cd /data/hwsec-students/"$USER"/assignment-2/
make clean
make all
srun -t 20:00 --pty /data/hwsec-students/"$USER"/assignment-2/grade.sh
