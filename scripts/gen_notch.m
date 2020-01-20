#!/usr/bin/env octave-cli --no-gui --quiet --norc
arg_list = argv();
if nargin != 3
    printf('Bad argument count\n')
    exit(1)
endif

sample_rate = str2num(arg_list{1});
nyquist = sample_rate / 2;
notch_width = str2num(arg_list{2});
notch_freq = str2num(arg_list{3});

pkg load signal;
[b, a] = pei_tseng_notch(notch_freq / nyquist, notch_width / nyquist);
if a(1) != 1 || b(1) != b(3) || a(2) != b(2)
    printf('Filter invariant check failed\n')
    exit(1)
endif
printf('// pei_tseng_notch(%d/(%d/2),%d/(%d/2))\n', notch_freq, sample_rate, notch_width, sample_rate)
printf('{\n')
format = '    .coeff_%s = DEFINE_COEFF(%+6.6f),\n';
printf(format, 'b0', b(1))
printf(format, 'b1', b(2))
printf(format, 'a2', a(3))
printf('},\n')

