/*
 * Copyright (c) 2012-2014 Darren Kulp
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

#define _XOPEN_SOURCE 600

#include "encode.h"

#include <math.h>
#include <stdio.h>

// TODO redefine POPCNT for different compilers than GCC
#define POPCNT(x) __builtin_popcount(x)

struct put_state {
    int last_quadrant;
    double last_sample;
    double adjust;
};

static int encode_sample(struct encode_state *s, double freq, double gain, double sample_index, struct put_state *state)
{
    double proportion = sample_index / s->audio.sample_rate;
    double radians = proportion * 2. * M_PI;
    double sample = sin(radians * freq) * gain;

    int rc = s->cb.put_samples(&s->audio, 1, &sample, s->cb.userdata);
    state->last_sample = sample;

    return rc ? 1 : -1;
}

static int encode_bit(struct encode_state *s, double freq, double gain, struct put_state *state)
{
    int samples = 0;

    double inverse = asin(state->last_sample / s->gain);
    switch (state->last_quadrant) {
        case 0: break;
        case 1: /* FALLTHROUGH */                       // mirror around pi/2
        case 2: inverse = M_PI - inverse; break;        // mirror around 3*pi/2
        case 3: inverse = 2 * M_PI + inverse; break;    // mirror around 2pi
        default: return 1;
    }
    double inverse_prop = inverse / (2 * M_PI);
    double samples_per_cycle = s->audio.sample_rate / freq;
    // sample_offset gets +1 because we need to start at the *next* sample,
    // otherwise a sample will be duplicated
    double sample_offset = inverse_prop * samples_per_cycle + 1;

    double sidx;
    for (sidx = sample_offset; sidx < SAMPLES_PER_BIT(&s->audio) + sample_offset + state->adjust; sidx++) {
        samples += encode_sample(s, freq, gain, sidx, state);
    }

    state->adjust -= (sidx - sample_offset) - SAMPLES_PER_BIT(&s->audio);

    double si = sidx - 1; // undo last iteration of for-loop
    // TODO explain what is happening here
    state->last_quadrant = (si - floor(si / samples_per_cycle) * samples_per_cycle) / samples_per_cycle * 4;

    return samples;
}

int encode_carrier(struct encode_state *s, size_t bit_times)
{
    int samples = 0;
    int rc = 0;

    struct put_state state = { .last_quadrant = 0 };

    for (unsigned bit_index = 0; rc >= 0 && bit_index < bit_times; bit_index++) {
        rc = encode_bit(s, s->audio.freqs[s->channel][1], s->gain, &state);
        if (rc >= 0) samples += rc; else return -1;
    }

    return 0;
}

int encode_bytes(struct encode_state *s, size_t byte_count, unsigned bytes[byte_count])
{
    int samples = 0;
    int rc = 0;

    struct put_state state = { .last_quadrant = 0 };
    for (unsigned byte_index = 0; byte_index < byte_count; byte_index++) {
        unsigned byte = bytes[byte_index];
        if (s->verbosity)
            printf("writing byte %#x\n", byte);

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.start_bits; bit_index++) {
            rc = encode_bit(s, s->audio.freqs[s->channel][0], s->gain, &state);
            if (rc >= 0) samples += rc; else return -1;
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            double freq = s->audio.freqs[s->channel][bit];
            rc = encode_bit(s, freq, s->gain, &state);
            if (rc >= 0) samples += rc; else return -1;
        }

        int oddness = POPCNT(byte) & 1;
        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.parity_bits; bit_index++) {
            // assume EVEN parity for now
            rc = encode_bit(s, s->audio.freqs[s->channel][oddness], s->gain, &state);
            if (rc >= 0) samples += rc; else return -1;
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.stop_bits; bit_index++) {
            rc = encode_bit(s, s->audio.freqs[s->channel][1], s->gain, &state);
            if (rc >= 0) samples += rc; else return -1;
        }
    }

    return samples;
}


