#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
zeros_file=$here/zeros.wav
ones_file=$here/ones.wav

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

$RUNS $hysteresis <(sox $zeros_file -t raw - | textify | $RMS $rms_samples) <(sox $ones_file -t raw - | textify | $RMS $rms_samples) |
    tail -n+$ignore_samples |
    $DECODE $offset

