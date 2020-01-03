#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
input_file=${1?"Specify an input file name"}
channel=0
threshold=10

source $here/bash_functions

for rms_samples in {4..28}
do
    for hysteresis in {3..15}
    do
        for offset in {0..20}
        do
            (
                out=$here/out/$rms_samples,$hysteresis,$offset
                if [[ ! -e $out ]] # assume existence implies previous completion
                then
                    $here/../listen $channel $rms_samples $threshold $hysteresis $offset < $input_file 2> /dev/null > $out
                fi
            ) &
        done
        wait
    done
done

