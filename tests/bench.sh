#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
input_file=$1

source $here/bash_functions

for rms_samples in {4..25}
do
    for hysteresis in {3..10}
    do
        for offset in {3..20}
        do
            for bw in 20 50 100
            do
                (
                    out=$here/out/$rms_samples,$hysteresis,$offset,$bw
                    if [[ ! -e $out ]] # assume existence implies previous completion
                    then
                        unpack $input_file | $here/../top $rms_samples $hysteresis $offset $(notch 1070 8000 $bw) $(notch 1270 8000 $bw) 2> /dev/null > $out
                    fi
                ) &
            done
            wait
        done
    done
done

