#!/usr/bin/env bash
here=$(dirname $0)
AVR_OPTFLAGS='-Os    $(LTO_FLAGS)' $here/sizes.sh | git notes --ref=sizes-small add -f -F -
AVR_OPTFLAGS='-Ofast $(LTO_FLAGS)' $here/sizes.sh | git notes --ref=sizes-fast  add -f -F -
