#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
input_file=$1

source $here/bash_functions

for rms_samples in {4..28}
do
    for hysteresis in {3..15}
    do
        for offset in {0..20}
        do
            for bw in 50 100 150 200
            do
                (
                    out=$here/out/$rms_samples,$hysteresis,$offset,$bw
                    if [[ ! -e $out ]] # assume existence implies previous completion
                    then
                        $here/../decode $rms_samples $hysteresis $offset $(notch 1070 8000 $bw) $(notch 1270 8000 $bw) < $input_file 2> /dev/null > $out
                    fi
                ) &
            done
            wait
        done
    done
done

