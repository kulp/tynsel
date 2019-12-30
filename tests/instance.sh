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

function textify ()
{
    perl -le 'local $/; print for map { unpack "s" } map { /\G(..)/gms } <>' "$@"
}

function notch_1270 () { sox "$@" biquad 0.96172 -1.04336 0.96172 1.00000 -1.04336 0.92344; } # [b,a]=pei_tseng_notch(1270/4000,100/4000)
function notch_1070 () { sox "$@" biquad 0.96153 -1.28304 0.96153 1.00000 -1.28304 0.92306; } # [b,a]=pei_tseng_notch(1070/4000,100/4000)

function invert () { sox "$@" vol -1; }

function pass_freq ()
{
    local freq=$1
    shift
    local input=$1
    sox -m <(notch_$freq $input -p | invert -p -p) "$@"
}

$RUNS $hysteresis <(pass_freq 1070 $input_file -b 16 -t raw - | textify | $RMS $rms_samples) <(pass_freq 1270 $input_file -b 16 -t raw - | textify | $RMS $rms_samples) |
    tail -n+$ignore_samples |
    $DECODE $offset

