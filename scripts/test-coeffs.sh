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

# The generated coefficient file carries one entry per frequency, in the order
# FREQUENCY_LIST in types.h emits them, so a channel's two filters are neighbours
# only at the right stride. Indexing that table by channel instead of by channel
# pair handed the decoder the 1270 Hz filter next to the 2025 Hz filter, one filter
# from each channel, and channel one then decoded nothing at all.

message='HELLO WORLD'
for channel in 0 1; do
    printf '%s' "$message" |
        "$here/../gen" -C "$channel" > "$temp/channel$channel-raw"
    "$here/../listen" -C "$channel" -W 7 -T 10 -H 10 -O 12 \
        < "$temp/channel$channel-raw" > "$temp/decoded$channel"
    # The demodulator has to come to a decision about the carrier lead-in before
    # it reads a character, so the decode has to be byte for byte the message.
    cmp <(printf '%s' "$message") "$temp/decoded$channel" ||
        fail "channel $channel did not round trip"
    echo "good: channel $channel round trips"
done

# Both channels round tripping is only meaningful while they stay distinguishable,
# so audio from one must not decode through the other's filter pair either.
printf '%s' "$message" | "$here/../gen" -C 1 > "$temp/fm-raw"
"$here/../listen" -C 0 -W 7 -T 10 -H 10 -O 12 < "$temp/fm-raw" > "$temp/fm-as-am"
printf '%s' "$message" | "$here/../gen" -C 0 > "$temp/am-raw"
"$here/../listen" -C 1 -W 7 -T 10 -H 10 -O 12 < "$temp/am-raw" > "$temp/am-as-fm"
test ! -s "$temp/fm-as-am" ||
    fail "channel one audio decodes through channel zero"
test ! -s "$temp/am-as-fm" ||
    fail "channel zero audio decodes through channel one"

echo "good: the channels keep separate filter pairs"
