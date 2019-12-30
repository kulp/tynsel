#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
input_file=${1?"Supply input filename as first parameter"}
shift

DECODE=${DECODE:-$here/decode.pl}
RMS=${RMS:-$here/rms.pl}
RUNS=${RUNS:-$here/runs.pl}

rms_samples=${1:-10}
hysteresis=${2:-7}
offset=${3:-15}
ignore_samples=${4:-5070}

source $here/bash_functions

if [[ -n ${FILTER:-} ]]
then
    function filter ()
    {
        local file=$1
        shift
        sox $file -t raw -e signed -r 8000 -b 16 - | unpack | $FILTER "$@" | pack
    }
fi

$RUNS $hysteresis <(pass_freq 1070 $input_file -b 16 -t raw - | unpack | $RMS $rms_samples) <(pass_freq 1270 $input_file -b 16 -t raw - | unpack | $RMS $rms_samples) |
    tail -n+$ignore_samples |
    $DECODE $offset

