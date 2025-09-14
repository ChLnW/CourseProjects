#!/bin/bash

cd "$(dirname "$0")"

if [[ "$@" == *"-h"* ]]; then
  echo "Usage: $0 [TASK [OTHER TASK [..]]"
  echo "   grades all tasks if non specified"
  exit 1
fi

nc="\x1b[0;0m"
TASK=""

pfx_erro="[\x1b[31;1m!${nc}]"
pfx_succ="[\x1b[32;1m*${nc}]"
pfx_warn="[\x1b[33mw${nc}]"
pfx_info="[\x1b[34mi${nc}]"

function FAIL { echo -e "${pfx_erro} $@"; }
function SUCC { echo -e "${pfx_succ} $@"; }
function WARN { echo -e "${pfx_warn} $@"; }
function INFO { echo -e "${pfx_info} $@"; }

if [[ ! "$(hostname)" =~ ee-tik-cn[0-9]{3} ]]; then
  WARN "This script is intended to run on a compute node!"
fi
INFO "Grading on host: $(hostname)"

function time_ms {
  date +%s%3N ## macos users: you need gdate (gnu core utils)
}

function random_address {
  N=0x$(xxd -l6 -p /dev/urandom)
  N=$((N & ~0xfff))
  N=$((N & ~(1<<47)))
  printf "0x%012x\n" $N
}

function levels {
  ADR=0x${1#0x}
  L1=$((($ADR>>12) & 0x1ff))
  L2=$((($ADR>>21) & 0x1ff))
  L3=$((($ADR>>30) & 0x1ff))
  L4=$((($ADR>>39) & 0x1ff))
  echo "$L1 $L2 $L3 $L4"
}

function diff_levels {
  read -a A <<< $(levels $1)
  read -a B <<< $(levels $2)
  D=(0 0 0 0)
  for l in {0..3}; do
    D[$l]=$((${A[$l]} ^ ${B[$l]}))
  done
  echo "${D[@]}"
}

function check_tlb {
  TASK=test__evict_tlb
  make clean $TASK > /dev/null
  OUTP=$(timeout 10s ./$TASK)
  if [ $? -eq 124 ]; then
    FAIL "$1 timed out..."
    return -1
  fi

  if ! egrep -q 'cached=[0-9]* tlbmiss=[0-9]*' <<< "$OUTP"; then
    FAIL "Unexpected output"
    FAIL "Got: $OUTP"
    return -1
  fi
  SUCC "Output is OK"
  return 0
}

function check_cache {
  TASK=test__evict_cache
  make clean $TASK > /dev/null

  OUTP=$(timeout 10s ./$TASK)
  if [ $? -eq 124 ]; then
    WARN "$1 timed out..."
    return -1
  fi

  if (($(egrep 'pgoff=0x... hit=[0-9]* miss=[0-9]*' <<< $OUTP | wc -l) != 64)); then
    FAIL "Unexpected output"
    return -1
  fi
  SUCC "Output is OK"
}

function test_anc {
  TASK=anc
  make clean $TASK > /dev/null

  if objdump -d ./$TASK | grep -q clflush; then
    FAIL "Do not use clflush or clflushopt instructions!"
  fi

  full_recovery=0 # recover 100%

  # Partal recovery means either
  # 1) one correct PML or
  # 2) at least 2 PML were within the correct CL
  part_recovery=0

  for round in {1..10}; do
    ADR=$(random_address)
    printf "${pfx_info}${pad} Round %02d -----------------------\n" "$round"

    t0=$(time_ms)
    OUTP=$(timeout 120s ./$TASK $ADR)
    if [ $? -eq 124 ]; then
      WARN "$1 timed out..."
    fi
    t=$(($(time_ms) - $t0))

    EXPECTED=${ADR#0x}
    ACTUAL=$(<<<$OUTP grep "PAGE=" | tail -n1 | sed 's/PAGE=0x\(.*\)$/\1/')
    INFO "Expected:         $ADR"
    INFO "Actual:           0x$ACTUAL"

    read -a D <<< $(diff_levels $EXPECTED $ACTUAL)

    msg="Time:             $(bc -l <<< "scale=2; $t/1000.0") s"
    if (( $t < 60000 )); then
      full=0
      for val in ${D[@]}; do
        full=$(($full + $val))
      done
      if (( $full == 0 )); then
        full_recovery=$(($full_recovery + 1))
      fi
      SUCC "$msg"
    else
      WARN "$msg"
    fi

    cl_ok=0
    for l in {0..3}; do
      cl_ok=$(($cl_ok + ((${D[0]} >> 3) == 0)))
    done

    if [[ ${D[0]} == 0 ]] || (( $cl_ok > 1 )); then
      part_recovery=$(($part_recovery + 1))
    fi
  done

  any_recovery=$(($full_recovery + $part_recovery))

  msg="Full recovery:    ${full_recovery}/10"
  if (( $full_recovery > 7 )); then
    SUCC "$msg"
    return 0
  elif (( $full_recovery > 3 )); then
    WARN "$msg"
    return 0
  else
    FAIL "$msg"
  fi

  msg="Partial recovery: ${part_recovery}/10"
  if (( $any_recovery > 3 )); then
    SUCC "$msg"
    return 0
  else
    FAIL "$msg"
    return -1
  fi
}

TO_CHECK=${@:-"cache tlb anc"}

if [[ $TO_CHECK == *"anc"* ]]; then
  INFO "=== ANC"
  if test_anc; then
    SUCC "Pass"
  else
    FAIL "Fail"
  fi
fi

if [[ $TO_CHECK == *"tlb"* ]]; then
  INFO "=== check test__evict_tlb"
  if check_tlb; then
    SUCC "Pass"
  else
    FAIL "Fail"
  fi
fi

if [[ $TO_CHECK == *"cache"* ]]; then
  INFO "=== check test__evict_cache"
  if check_cache; then
    SUCC "Pass"
  else
    FAIL "Fail"
  fi
fi

if [[ 0 == "$(cat ./figures/pml1.pdf | wc -c)" ]]; then
  WARN "Don't forget ./figures/pml1.pdf"
fi

if [[ 0 == "$(cat ./figures/pml234.pdf | wc -c)" ]]; then
  WARN "Don't forget ./figures/pml234.pdf"
fi
