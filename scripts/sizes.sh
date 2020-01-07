#!/usr/bin/env bash
set -euo pipefail
here=$(dirname $0)

function summate ()
{
    let sum=0
    while read addend; do let sum=$sum+$addend; done
    echo $sum
}

function section_size ()
{
    avr-readelf --wide --section-details $1 | fgrep --after=1 "$2" | tail -n1 |
        (read kind addr off size rest ; echo $(( 0x$size )))
}

out=$(mktemp -d)
echo >&2 "Output: $out"

touch $out/started
sleep 1 # let the filesystem catch up with its unpredictable timestamp asynchronicity

# -fstack-usage does not seem to work with LTO on, so build without it first
make --directory=$here/.. --jobs --always-make LTO=0 avr-top &> /dev/null
cp -p avr-top $out/avr-top.no-lto
make --directory=$here/.. --jobs --always-make LTO=1 avr-top &> /dev/null
cp -p avr-top $out/avr-top.lto

find $here/.. -maxdepth 1 -name '*.su' -newer $out/started -exec cp -p {} $out/ \;

stack=$(cut -f2 $out/*.su | summate)
text=$(section_size $out/avr-top.lto .text)
data=$(section_size $out/avr-top.lto .data)
bss=$(section_size $out/avr-top.lto .bss)

printf ".text: %6d\n" $text
printf ".data: %6d\n" $data
printf ".bss:  %6d\n" $bss
printf "stack: %6d\n" $stack
echo   "-------------"
printf "RAM:   %6d\n" $(( data + bss + stack ))
printf "Flash: %6d\n" $text

