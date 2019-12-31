#!/usr/bin/env octave-cli --no-gui --quiet --norc
arg_list = argv();
if nargin != 3
    printf('Bad argument count\n')
    exit(1)
endif

notch_freq = str2num(arg_list{1});
sample_rate = str2num(arg_list{2});
nyquist = sample_rate / 2;
notch_width = str2num(arg_list{3});

pkg load signal;
[b, a] = pei_tseng_notch(notch_freq / nyquist, notch_width / nyquist);
printf('// pei_tseng_notch(%d/(%d/2),%d/(%d/2))\n', notch_freq, sample_rate, notch_width, sample_rate)
printf('{\n')
format = '    .%c = { FLOAT_TO_COEFF(%+6.6ff), FLOAT_TO_COEFF(%+6.6ff), FLOAT_TO_COEFF(%+6.6ff) },\n';
printf(format, 'b', b(1), b(2), b(3))
printf(format, 'a', a(1), a(2), a(3))
printf('}\n')

