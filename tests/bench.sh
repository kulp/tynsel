#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
input_file=$1

for rms_samples in {9..11}
do
    for hysteresis in {6..8}
    do
        for offset in {14..16}
        do
            (
                out=out/$rms_samples,$hysteresis,$offset
                if [[ ! -e $out ]] # assume existence implies previous completion
                then
                    $here/instance.sh $input_file $rms_samples $hysteresis $offset 2> /dev/null > $out
                fi
            ) &
        done
        wait
    done
done

