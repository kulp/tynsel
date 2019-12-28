#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)

for rms_samples in {4..6}
do
    for hysteresis in {4..6}
    do
        for offset in {9..15}
        do
            (
                out=out/$rms_samples,$hysteresis,$offset
                if [[ ! -e $out ]] # assume existence implies previous completion
                then
                    $here/instance.sh $rms_samples $hysteresis $offset 2> /dev/null > $out
                fi
            ) &
        done
        wait
    done
done

