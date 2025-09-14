#!/bin/bash

# this script runs the ./test_drama exe on all nodes
# so that i can run histogram.py and get all of their tresholds and graphs

# List of nodes
# nodes=("cn001" "cn002" "cn003" "cn004" "cn005" "cn006" "cn007" "cn008" "cn009"
#   "cn010" "cn011" "cn012" "cn013" "cn014" "cn015" "cn016" "cn017" "cn018"
#   "cn019" "cn020" "cn021" "cn022")

# nodes=("cn008")
# nodes=("cn010")
# nodes=("cn015")
nodes=("cn017")

# Loop through the list of nodes and submit a job for each
for node in "${nodes[@]}"; do
  echo "Submitting job to $node"
  sbatch --nodelist=ee-tik-$node <<EOF
#!/bin/bash
#SBATCH --job-name=test_job_$node
#SBATCH --time=00:10:00
#SBATCH --output=result.out
#SBATCH --ntasks=1

# Change to the directory where the script is located
cd "$PWD" || exit

# Run the commands
make clean
make test__blacksmith
./test__blacksmith -d
EOF

done
