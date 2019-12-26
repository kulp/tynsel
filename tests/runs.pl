#!/usr/bin/env perl
use strict;
use warnings;
use feature 'say';

my $threshold = shift
    // die "Supply a threshold count as the first argument";

my %values = qw(
    >  1
    =  0
    < -1
);

my $count = 0;

my ($min, $max) = (-$threshold, $threshold);

while (<>) {
    chomp;
    my $inc = $values{$_} // die "invalid character";
    $count += $inc;
    $count = $max if $count > $max;
    $count = $min if $count < $min;
    say $count;
}

