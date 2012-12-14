#!/usr/bin/env perl
use strict;

my $trials = 1000;
my $maxlen = 100;
my $fname = "trial.wav";
my $rate = 8000;
my $genopts = "-G .5";

for my $trial (1 .. $trials) {
    my $length = int rand $maxlen;
    print "Trial $trial of $trials, $length elements ...\n";

    my @bytes = map { int rand 256 } 1 .. $length;
    system("./gen -s $rate -o $fname $genopts @bytes");
    system("./g711 $fname $fname.g711.wav");
    my @lines = (qx(./fft -s $rate $fname.g711.wav 2> /dev/null))[0 .. scalar $#bytes];
    chomp, s/^.*= //, $_ = hex $_ for @lines;
    check(\@lines, \@bytes);
}

sub check
{
    my @errors;
    my $len = $#{ $_[0] };
    for my $i (0 .. $len) {
        my ($a, $b) = ($_[0][$i], $_[1][$i]);
        if ($a != $b) {
            warn sprintf("Mismatch at element %d of %d : 0x%x vs 0x%x", $i, $len, $a, $b);
            push @errors, $i;
        }
    }

    die sprintf "%d total errors", scalar @errors if @errors;
}

