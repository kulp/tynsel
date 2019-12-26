#!/usr/bin/env perl
use strict;
use warnings;
use feature 'say';

open my $fa, shift;
open my $fb, shift;

while (not eof($fa) and not eof($fb)) {
    my $sa = do { local $_ = <$fa>; chomp; int $_ };
    my $sb = do { local $_ = <$fb>; chomp; int $_ };

    say ">" if $sa >  $sb;
    say "=" if $sa == $sb;
    say "<" if $sa <  $sb;
}

