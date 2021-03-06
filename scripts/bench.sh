#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)
channel=0
bits_per_sample=16
sample_rate=8192
baud_rate=300
bits_per_word=11
byte_count=100
gain=0.8

outdir=$(mktemp -d)
echo "outdir: $outdir"

source $here/bash_functions

function random_bytes ()
{
    LC_ALL=C tr -dc '[[:cntrl:][:space:][:graph:]]' < /dev/urandom | dd bs=${1:-100} count=1 2>/dev/null || true
}

function noise ()
{
    local seconds=${1:-1}
    local vol=${2:-1}
    sox -n -r $sample_rate -p synth $seconds noise vol $vol
}

function generate_audio ()
{
    local input_file=$1
    $here/../gen -C $channel -G $gain -F $input_file
}

function mix ()
{
    local file=$1
    local noise_file=$2
    sox -m -t raw -r $sample_rate -e signed -b $bits_per_sample "$@" -t raw -
}

function compute_duration ()
{
    local bytes=$1
    # `gen` generates about 576 bit-times of padding, half on each end
    bc <<<"scale=3;($bytes*$bits_per_word+576)/$baud_rate"
}

for run in {1..10}
do
    rand=$(mktemp $outdir/random_bytes.XXXXXX)
    stem=$(basename $rand)
    random_bytes $byte_count > $rand
    generate_audio $rand > $rand.audio
    for noise_level in 0 0.1 0.2 0.4 0.8
    do
        noise=$(mktemp $outdir/noise-$noise_level.XXXXXX)
        noise $(compute_duration $byte_count) $noise_level > $noise
        mix $rand.audio $noise > $rand.audio.noised-$noise_level

        for threshold in 1 16 256 1024
        do
            for window in {5..8} # 8 is the hardcoded maximum in the embedded code at this time
            do
                for hysteresis in {4..11}
                do
                    for offset in {1..15}
                    do
                        (
                            out=$outdir/bits_per_sample$bits_per_sample/chan$channel/run$run.$stem/noise$noise_level/window$window/threshold$threshold/hysteresis$hysteresis/offset$offset/$stem.decoded
                            mkdir -p $(dirname $out)
                            if [[ ! -e $out ]] # assume existence implies previous completion
                            then
                                $here/../listen -C $channel -W $window -T $threshold -H $hysteresis -O $offset < $rand.audio.noised-$noise_level 2> /dev/null > $out
                            fi
                        ) &
                    done
                    wait
                done
            done
        done
    done
done

