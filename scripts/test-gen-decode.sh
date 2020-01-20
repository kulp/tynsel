#!/bin/bash
set -euo pipefail
temp=$(mktemp -d)
${TRAP:-trap} "rm -rf $temp" EXIT
LC_ALL=C tr -dc '[[:alnum:] [:punct:]\n]' < /dev/urandom | cut -b1-50 | head -n1 > $temp/str || true
make --environment-overrides -Bj && ./gen -F $temp/str |
    tee $temp/gen-raw |
    ./listen -C 0 -W 7 -T 10 -H 10 -O 12 |
    tee $temp/decoded |
    cmp $temp/str &&
    echo good || (echo bad: $temp ; false)
