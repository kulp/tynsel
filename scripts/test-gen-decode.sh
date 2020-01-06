#!/bin/bash
set -euo pipefail
temp=$(mktemp -d)
${TRAP:-trap} "rm -rf $temp" EXIT
LC_ALL=C tr -dc '[[:alnum:] [:punct:]\n]' < /dev/urandom | cut -b1-50 | head -n1 > $temp/str || true
make -Bj && ./gen < $temp/str |
    tee $temp/gen-raw |
    ./listen 0 7 10 10 12 |
    tee $temp/decoded |
    cmp $temp/str &&
    echo good || (echo bad: $temp ; false)
