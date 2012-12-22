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

#define PERBIN          ((double)s->audio.sample_rate / size)

static size_t get_max_magnitude(struct decode_state *s, size_t size, double samples[size], double low, double high)
{
    size_t maxi = 0;
    double max = -1;
    for (size_t i = 0; i < size; i++) {
        double mag = samples[i];
        if (s->verbosity > 3) {
            printf("magnitude[%zd] = { %6.2f }\n", i, mag);
        }
        if (mag > max && ((low - PERBIN) / PERBIN) <= i && i <= ((high + PERBIN) / PERBIN)) {
            max = mag;
            maxi = i;
        }
    }

    return maxi;
}

static void get_nearest_freq(struct decode_state *s, double freq, int *ch, int *f)
{
    double min = DBL_MAX;

    if (s->verbosity)
        printf("midpoint frequency is %4.0f\n", freq);

    unsigned minch = 0,
             maxch = 1;

    // If we were given a channel, contrain to that one
    if (*ch >= 0 && *ch < 2)
        minch = maxch = *ch;

    for (unsigned chan = minch; chan <= maxch; chan++) {
        for (unsigned i = 0; i < 2; i++) {
            double t = fabs(freq - s->audio.freqs[chan][i]);
            if (t < min) {
                min = t;
                *ch = chan;
                *f = i;
            }
        }
    }
}

int decode_bit_naive(struct decode_state *s, size_t size, double samples[size], int *channel, int *bit, double *prob)
{
    unsigned minch = 0,
             maxch = 1;

    // If we were given a channel, contrain to that one
    if (*channel >= 0 && *channel < 2)
        minch = maxch = *channel;

    size_t maxi = get_max_magnitude(s, size, samples, s->audio.freqs[minch][0], s->audio.freqs[maxch][1]);

    if (s->verbosity) {
        printf("bucket with greatest magnitude was %zd, which corresponds to frequency range [%4.0f, %4.0f)\n",
                maxi, PERBIN * maxi, PERBIN * (maxi + 1));
    }

    double freq = maxi * PERBIN + (PERBIN / 2.);
    get_nearest_freq(s, freq, channel, bit);
    *prob = 1; // XXX give a real probability

    return 0;
}

