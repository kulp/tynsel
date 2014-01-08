#!/usr/bin/env perl
#
# Copyright (c) 2012-2014 Darren Kulp
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#

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
    my @lines = (qx(./suite 0 $fname 2> /dev/null))[0 .. scalar $#bytes];
    chomp, s/^char '.' \((\d+)\)/$1/ for @lines;
    check(\@lines, \@bytes);
}

sub check
{
    my @errors;
    my $len1 = scalar @{ $_[0] };
    my $len2 = scalar @{ $_[1] };
    if ($len1 != $len2) {
        die "Length mismatch : $len1 != $len2";
    } else {
        for my $i (0 .. $len1 - 1) {
            my ($a, $b) = ($_[0][$i], $_[1][$i]);
            if ($a != $b) {
                warn sprintf("Mismatch at element %d of %d : 0x%x vs 0x%x", $i, $len1 - 1, $a, $b);
                push @errors, $i;
            }
        }
    }

    die sprintf "%d total errors", scalar @errors if @errors;
}

