/*
 * Copyright (c) 2012 Darren Kulp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "decode.h"
#include "common.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>

static double rms(size_t size, const double samples[size])
{
    double sum = 0.;

    for (size_t i = 0; i < size; i++)
        sum += samples[i] * samples[i];

    return sqrt(sum / size);
}

// scalar result of multiplying arrays `a` and `b`
static double mult(size_t size, const double a[size], const double b[size])
{
    double sum = 0.;

    for (size_t i = 0; i < size; i++)
        sum += a[i] * b[i];

    return sum;
}

static int correlate(size_t size, const double /* in */ a[size], const double /* in */ b[size], double /* out */ c[size])
{
    for (size_t i = 0; i < size; i++)
        c[i] = mult(size - i, a, &b[i]);

    return 0;
}

int decode_bit_correlate(struct decode_state *s, size_t size, const double samples[size], int *channel, int *bit, double *prob)
{
    unsigned minch = 0,
             maxch = 1;

    // If we were given a channel, contrain to that one
    if (*channel >= 0 && *channel < 2)
        minch = maxch = *channel;

    double maxmag = 0;
    double energies[maxch - minch + 1][2];

    for (unsigned ch = minch; ch <= maxch; ch++) {
        for (unsigned idx = 0; idx < 2; idx++) {
            double pure[size], temp[size];
            // TODO cache pure waves
            for (size_t si = 0; si < size; si++) {
                pure[si] = sin(2 * M_PI * si / s->audio.sample_rate * s->audio.freqs[ch][idx]);
            }
            correlate(size, samples, pure, temp);
            double energy = energies[ch][idx] = rms(size, temp);
            if (energy > maxmag) {
                *channel = ch;
                *bit = idx;
                *prob = 1 - (maxmag / energy);
                maxmag = energy;
            }
        }
    }

    return 0;
}

