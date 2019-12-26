#!/usr/bin/env perl
use strict;
use warnings;
use feature 'say';

my $offset = shift
    // die "Supply a offset (in samples) from the start bit edge as the first argument";

my $bitwidth = 27; # approximately (8000Hz)/(300 baud)

my $last;
my $edge = -$bitwidth;
my $bit = 0;
my $byte = 0;
while (<>) {
    chomp;
    next unless defined $last;
    my $here = int;
    if ($bit == 0 && $here >= 0 && $last < 0) {
        if ($. - $edge < $bitwidth) {
            warn "found an edge at line $., sooner than expected (last edge was $edge)";
        }
        # found a positive-going edge
        $edge = $.;
    }

    if ($edge + $offset + $bit * $bitwidth == $.) {
        # sample here
        my $found = $here >= 0 ? 0 : 1;

        if ($bit == 0) {
            # start bit, skip
            if ($found != 0) {
                warn "Start bit is not 0";
                $byte = 0;
                $bit = 0;
                $edge = 0;
                next;
            }
        } elsif ($bit == 8) {
            # parity, skip
        } elsif ($bit > 8) {
            if ($found != 1) {
                warn "Stop bit was not 1";
                $byte = 0;
                $bit = 0;
                $edge = 0;
                next;
            }
        } else {
            $byte |= ($found << ($bit - 1));
        }

        say "found $found";

        if ($bit >= 10) {
            printf "Byte : 0b%07b (`%s`)\n", $byte, chr $byte;
            $byte = 0;
            $bit = 0;
            $edge = 0;
        } else {
            $bit++;
        }
    }
} continue {
    $last = int;
}

