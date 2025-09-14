#!/bin/bash

#SBATCH --job-name test_job
#SBATCH --time 00:10:00
#SBATCH --output result.out
#SBATCH --ntasks=1
#SBATCH --nodelist=ee-tik-cn002

# assuming this runs in /data/hwsec-students/<NAME>/HWSec/assignment-4 for simplicity
# will print results to result.out

cd "$PWD" &&
  make test__drama &&
  ./test__drama -t 500
