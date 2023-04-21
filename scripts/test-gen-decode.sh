#!/bin/bash
set -euo pipefail
temp=$(mktemp -d)
here="$(dirname "$0")"
${TRAP:-trap} "rm -rf $temp" EXIT
LC_ALL=C tr -dc '[[:alnum:] [:punct:]\n]' < /dev/urandom | cut -b1-50 | head -n1 > $temp/str || true
$here/../gen -F $temp/str |
    tee $temp/gen-raw |
    $here/../listen -C 0 -W 7 -T 10 -H 10 -O 12 |
    tee $temp/decoded |
    cmp $temp/str /dev/stdin &&
    echo good || (echo bad: $temp ; false)
