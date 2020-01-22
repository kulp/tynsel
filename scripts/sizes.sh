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

make --directory=$here/.. --jobs --always-make LTO=1 avr-top &> /dev/null
cp -p avr-top $out/avr-top.lto

text=$(section_size $out/avr-top.lto .text)
data=$(section_size $out/avr-top.lto .data)
bss=$(section_size $out/avr-top.lto .bss)
eeprom=$(section_size $out/avr-top.lto .eeprom)

printf ".text:   %6d\n" $text
printf ".data:   %6d\n" $data
printf ".bss:    %6d\n" $bss
printf ".eeprom: %6d\n" $eeprom
