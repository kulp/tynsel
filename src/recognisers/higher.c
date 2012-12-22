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

int decode_bit_higher(struct decode_state *s, size_t size, double samples[size], int *channel, int *bit, double *prob)
{
    unsigned minch = 0,
             maxch = 1;

    // If we were given a channel, contrain to that one
    if (*channel >= 0 && *channel < 2)
        minch = maxch = *channel;

    double maxmag = -1.;

    for (unsigned ch = minch; ch <= maxch; ch++) {
        double freq[] = { s->audio.freqs[ch][0], s->audio.freqs[ch][1] };
        double mag[] = { samples[ (size_t)(freq[0] / PERBIN) ], samples[ (size_t)(freq[1] / PERBIN) ] };

        int idx = mag[1] > mag[0];
        int relevant = mag[idx] > maxmag;
        if (relevant) {
            maxmag = mag[idx];
            *channel = ch;
            *bit = idx;
            // "Probability" is the ratio of difference to total magnitude.
            *prob = (mag[idx] - mag[1 - idx]) / mag[idx];
        }
    }

    return 0;
}


