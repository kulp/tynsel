#!/bin/bash
set -euo pipefail
temp=$(mktemp -d)
here="$(dirname "$0")"
${TRAP:-trap} "rm -rf $temp" EXIT
head -c500 /dev/urandom | LC_ALL=C tr -c '[:graph:]' ' ' > $temp/str || true
$here/../gen -F $temp/str |
    tee $temp/gen-raw |
    $here/../listen -C 0 -W 7 -T 10 -H 10 -O 12 |
    tee $temp/decoded |
    cmp $temp/str /dev/stdin &&
    echo good || (echo bad: $temp ; false)
