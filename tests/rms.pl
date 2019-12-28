#!/usr/bin/env perl
use strict;
use warnings;
use feature 'say';

use List::Util qw(sum);

# First argument is window size, remainder are input files
my $n = shift;

sub square ($) { $_[0] * $_[0] }

my @list;
while (<>) {
    push @list, int;
    say sum map square($_), $list[-$n..-1] if $n <= scalar @list;
}

