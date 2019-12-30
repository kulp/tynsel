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

function notch ()
{
    local notch_freq=${1?"Supply a notch frequency"}
    local nyquist=$(( ${2:-8000} / 2 ))
    local notch_width=${3:-100}

    octave-cli --eval "pkg load signal; [b,a]=pei_tseng_notch($notch_freq/$nyquist,$notch_width/$nyquist); disp([b a])"
}

function invert () { sox "$@" vol -1; }

function pass_freq ()
{
    local freq=$1
    shift
    local input=$1
    sox -m <(sox $input -p biquad $(notch $freq) | invert -p -p) "$@"
}

$RUNS $hysteresis <(pass_freq 1070 $input_file -b 16 -t raw - | textify | $RMS $rms_samples) <(pass_freq 1270 $input_file -b 16 -t raw - | textify | $RMS $rms_samples) |
    tail -n+$ignore_samples |
    $DECODE $offset

