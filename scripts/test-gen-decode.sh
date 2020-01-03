#!/bin/bash
set -euo pipefail
# The length of the test string is chosen to work around (in the older,
# pre-integer implementation) a discontinuity between encoding bytes and
# encoding the trailing carrier, a discontinuity which can look like a start
# bit
string="hello there, world !!"
temp=$(mktemp -d)
${TRAP:-trap} "rm -rf $temp" EXIT
echo $string > $temp/str
make -Bj && ./gen $(perl -e 'print join " ", map unpack("c"), map { split // } <>' $temp/str) |
    tee $temp/gen-raw |
    ./listen 0 7 10 10 12 |
    tee $temp/decoded |
    cmp $temp/str &&
    echo good || (echo bad: $temp ; false)
