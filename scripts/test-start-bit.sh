#!/bin/bash
set -euo pipefail
temp=$(mktemp -d)
here="$(dirname "$0")"
${TRAP:-trap} "rm -rf $temp" EXIT

fail ()
{
    echo "bad: $1, $temp"
    exit 1
}

# The demodulator integrates how much better one channel's filter fares than the
# other's, and that integral starts at zero while the filters are still warming up.
# A first comparison that favours the wrong filter and then corrects itself looks
# like a rise through zero, which is what a start bit looks like, so the caller
# gets a character it never sent in front of the message. Windows two, three and
# five used to do that on channel zero; the windows the decoder was tuned with
# happen to warm up without the false edge, which is why this sweeps the window
# instead of using one.

for chan in 0 1; do
    printf 'HELLO WORLD' | "$here/../gen" -C "$chan" > "$temp/message-raw"
    printf '' | "$here/../gen" -C 0 > "$temp/carrier-raw"

    for window in 2 3 4 5 6 7 8; do
        for offset in 4 12; do
            # A carrier stream carries no start bit, so there is nothing to decode.
            "$here/../listen" -C "$chan" -W "$window" -T 10 -H 10 -O "$offset" \
                < "$temp/carrier-raw" > "$temp/carrier-out"
            test ! -s "$temp/carrier-out" ||
                fail "carrier decodes as a character at window $window offset $offset"

            # The message has to come back byte for byte, with nothing in front of it.
            "$here/../listen" -C "$chan" -W "$window" -T 10 -H 10 -O "$offset" \
                < "$temp/message-raw" > "$temp/message-out"
            cmp <(printf 'HELLO WORLD') "$temp/message-out" ||
                fail "message does not round trip at window $window offset $offset"
        done
    done
done

echo "good: nothing is read out of the carrier lead-in"
