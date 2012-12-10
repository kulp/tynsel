#!/usr/bin/env perl
use strict;

my $trials = 1000;
my $maxlen = 10;
my $fname = "trial.wav";
my $rate = 8000;

for my $trial (1 .. $trials) {
    print "Trial $trial of $trials ...\n";

    my $length = rand $maxlen;
    my @bytes = map { int rand 256 } 1 .. $length;
    system("./gen -s $rate -o $fname @bytes");
    my @lines = (qx(./fft -s $rate $fname 2> /dev/null))[0 .. scalar $#bytes];
    chomp, s/^.*= //, $_ = hex $_ for @lines;
}

sub check
{
    for my $i (0 .. $#{ $_[0] }) {
        die unless $_[0][$i] == $_[1][$i];
    }
}

