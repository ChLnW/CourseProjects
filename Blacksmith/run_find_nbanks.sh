#!/bin/bash

if [ -z "$1" ]; then
  echo "Usage: $0 <host_number>"
  exit 1
fi

# Format the host number (e.g., 22 becomes ee-tik-cn022)
HOST_NUM=$(printf "%03d" "$1")
HOSTNAME="ee-tik-cn$HOST_NUM"

if ! scontrol show nodes | grep -q "$HOSTNAME"; then
  echo "Host $HOSTNAME not found or unavailable."
  exit 1
fi

#
THRESHOLD=$(awk -F, -v host="$HOSTNAME" '$1 == host {print $2}' ./data/thresholds.csv)

if [ -z "$THRESHOLD" ]; then
  echo "No threshold found for host $HOSTNAME."
  exit 1
fi

cat <<EOF >slurm_script.sh
#!/bin/bash

#SBATCH --job-name=test_job
#SBATCH --time 00:10:00
#SBATCH --output=result.out
#SBATCH --ntasks=1
#SBATCH --nodelist=${HOSTNAME}

cd "\$PWD" &&
  make test__drama &&
  ./test__drama -t $THRESHOLD

EOF

sbatch slurm_script.sh

rm slurm_script.sh

echo "Job submitted for host ${HOSTNAME} with threshold ${THRESHOLD}."
