#!/bin/bash

VERBOSE=false
TIMES=10

if [[ "$@" == *"-h"* ]]; then
  echo "Usage: $0 [-t TIMES] [-v] [TASK [OTHER TASK [..]]"
  echo "   grades all tasks by running them $TIMES times if none specified"
  echo "   -v verbose"
  echo "   -h help"
  exit 1
fi

while getopts "vt:" arg; do
  if [[ $arg == "t" ]]; then
    TIMES=$OPTARG
  fi
  if [[ $arg == "v" ]]; then
    VERBOSE=true
  fi
done
shift $((OPTIND-1))
TO_CHECK=${@:-"meltdown_segv meltdown_tsx meltdown_spectre"}

# initialize a new log file
date > log
make clean

nc="\x1b[0;0m"
pfx_fail="[\x1b[31;1m!${nc}]"
pfx_succ="[\x1b[32;1m+${nc}]"
pfx_warn="[\x1b[33mW${nc}]"
pfx_info="[\x1b[34m-${nc}]"
function ERR  { echo -e "$pfx_fail $@"; }
function SUCC { echo -e "$pfx_succ $@"; }
function WARN { echo -e "$pfx_warn $@"; }
function INFO { echo -e "$pfx_info $@"; }

function ERRF  { printf "$pfx_fail $1" ${@:2}; }
function SUCCF { printf "$pfx_succ $1" ${@:2}; }
function WARNF { printf "$pfx_warn $1" ${@:2}; }
function INFOF { printf "$pfx_info $1" ${@:2}; }

if [[ ! "$(hostname)" =~ ee-tik-cn[0-9]{3} ]]; then
  WARN "This script is intended to run on a compute node!"
fi
INFO "Grading on host: $(hostname)"

function time_ms {
  date +%s%3N ## macos users: you need gdate (gnu core utils)
}

function newsecret() {
  secret=`head -c 10 </dev/urandom | md5sum | awk '{ print $1 }'`
  if [ `echo -n $secret | wc -c` != 32 ]; then
    ERR "Expected secret to be 32 bytes."
    exit 1
  fi
  SUCC "Generated secret: $secret"
  echo $secret >/dev/wom
}

function run_test {
  TASK=$1
  make $TASK > /dev/null
  echo "****** TESTING $TASK ****************" | tee -a log
  newsecret
  points=0
  EXPECTED="$secret"
  secret_len=${#EXPECTED}
  MAX_POINTS=$((secret_len * TIMES))

  exec_times=()

  $VERBOSE INFO "Expected:    $EXPECTED" | tee -a log

  $VERBOSE || printf "${pfx_info} Test result: "
  for i in $(seq $TIMES); do
    count=0

    t0=$(time_ms)
    OUTP=$(timeout 10s ./$TASK)
    if [ $? -eq 124 ]; then
      WARN "$1 timed out..."
      continue
    fi
    t=$(time_ms)
    exec_times+=( $((t - t0)) )

    >>log echo "YOUR BINARY OUTPUT >>>>>>>>"
    >>log echo "$OUTP"
    >>log echo "<<<<<<<<"

    ACTUAL=$(<<<$OUTP grep "SECRET=" | tail -n1 | sed -rn 's/SECRET=(.*)$/\1/p')
    $VERBOSE && printf  "$pfx_info "
    for (( i=0; i<$secret_len; i+=1 )); do
      expected_byte="${EXPECTED:$i:1}"
      actual_byte="${ACTUAL:$i:1}"

      if [[ "$expected_byte" == "$actual_byte" ]]; then
        count=$((count + 1))
        $VERBOSE && printf "\x1b[32m%s\x1b[0m" "$actual_byte"
      else
        $VERBOSE && printf "\x1b[31m%s\x1b[0m" "${actual_byte:-.}"
      fi
    done
    $VERBOSE && echo " ($count/$secret_len)"
    if ! $VERBOSE; then
      if (( $count == $secret_len )); then
        printf "\x1b[32m.\x1b[0m"
      elif (( $count > 10 )); then
        printf "\x1b[33m.\x1b[0m"
      else
        printf "\x1b[31m.\x1b[0m"
      fi
    fi
    points=$((points + count))
  done
  $VERBOSE || echo

  accuracy=$(($((100 * points))  / $((MAX_POINTS))))

  INFOF "Task: %16s\n" "$TASK"
  INFOF "Rounds: %14d\n" $TIMES

  if (( accuracy >= 95 )); then
    SUCCF "Accuracy: %11d%%\n" $accuracy
  else
    ERRF "Accuracy: %11d%%\n" $accuracy
  fi

  IFS=$'\n' s=($(sort -n <<< "${exec_times[*]}")); unset IFS
  function to_sec { bc -l <<< "scale=2; $1/1000.0"; }
  med_ms=${s[$((N/2))]}
  time_msg=$(printf "Time (min/med/max): %0.2fs/%0.2fs/%0.2fs\n" $(to_sec ${s[0]}) $(to_sec $med_ms) $(to_sec ${s[-1]}))
  if (( $med_ms > 1000 )) ; then
    ERR "$time_msg"
  else
    SUCC "$time_msg"
  fi
  return $(( accuracy >= 95 ))
}

for bin in meltdown_segv meltdown_tsx meltdown_spectre; do
  if [[ $TO_CHECK == *"$bin"*  ]]; then
    run_test $bin
  fi

  if objdump -d ./$bin | grep -q xbegin; then
    if [[ $bin != "meltdown_tsx" ]]; then
      WARN "$bin: should not use transactions!"
    fi
  else
    if [[ $bin == "meltdown_tsx" ]]; then
      WARN "$bin: does not use transactions!"
    fi
  fi
done

if [[ 0 == "$(cat ./figures/meltdown.pdf | wc -c)" ]]; then
  WARN "Don't forget ./figures/meltdown.pdf!"
fi
