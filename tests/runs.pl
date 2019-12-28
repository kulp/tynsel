#!/usr/bin/env perl
use strict;
use warnings;
use feature 'say';

my $threshold = shift
    // die "Supply a threshold count as the first argument";

my $count = 0;

my ($min, $max) = (-$threshold, $threshold);

open my $fa, shift;
open my $fb, shift;

while (not eof($fa) and not eof($fb)) {
    my $sa = do { local $_ = <$fa>; chomp; int $_ };
    my $sb = do { local $_ = <$fb>; chomp; int $_ };

    my $inc = $sa <=> $sb;
    $count += $inc;
    $count = $max if $count > $max;
    $count = $min if $count < $min;
    say $count;
}

