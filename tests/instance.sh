#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
zeros_file=$here/z
ones_file=$here/o

DECODE=${DECODE:-$here/decode.pl}

rms_samples=${1:-10}
hysteresis=${2:-7}
offset=${3:-15}
ignore_samples=${4:-5070}

$here/compare.pl <($here/rms.pl $rms_samples $zeros_file) <($here/rms.pl $rms_samples $ones_file) |
    $here/runs.pl $hysteresis |
    tail -n+$ignore_samples |
    $DECODE $offset

