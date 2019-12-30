#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
zeros_file=$here/zeros.raw
ones_file=$here/ones.raw

DECODE=${DECODE:-$here/decode.pl}
RMS=${RMS:-$here/rms.pl}
RUNS=${RUNS:-$here/runs.pl}

rms_samples=${1:-10}
hysteresis=${2:-7}
offset=${3:-15}
ignore_samples=${4:-5070}

function textify ()
{
    perl -le 'local $/; print for map { unpack "s" } map { /\G(..)/gms } <>' "$@"
}

$RUNS $hysteresis <(textify $zeros_file | $RMS $rms_samples) <(textify $ones_file | $RMS $rms_samples) |
    tail -n+$ignore_samples |
    $DECODE $offset

