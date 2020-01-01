/*
 * Copyright (c) 2012-2020 Darren Kulp
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

#include "encode.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define SAMPLES_PER_BIT(a) ((double)(a)->sample_rate / (a)->baud_rate)

// TODO redefine POPCNT for different compilers than GCC
#define POPCNT(x) __builtin_popcount(x)

#define TEST_BIT(Word,Index) (((Word) & (1 << (Index))) ? -1 : 0)

static bool encode_sample(SAMPLE_STATE *s, PHASE_STEP step, DATA_TYPE *out)
{
    uint8_t top = s->phase >> PHASE_FRACTION_BITS;
    DATA_TYPE  half    = TEST_BIT(top, 7);
    PHASE_TYPE quarter = TEST_BIT(top, 6);
    uint8_t lookup = (top ^ quarter) % TABLE_SIZE;

    s->phase += step;

    *out = (*s->quadrant)[lookup] ^ half;
    return true; // this function is infallible
}

static inline PHASE_STEP get_phase_step(FREQ_TYPE freq)
{
    return MINOR_PER_CYCLE * freq / SAMPLE_RATE;
}

static inline FREQ_TYPE get_frequency(enum channel channel, enum bit bit)
{
    static const FREQ_TYPE freqs[CHAN_max][BIT_max] = {
        [CHAN_ZERO][BIT_ZERO] = 1070,
        [CHAN_ZERO][BIT_ONE ] = 1270,
        [CHAN_ONE ][BIT_ZERO] = 2025,
        [CHAN_ONE ][BIT_ONE ] = 2225,
    };

    return freqs[channel][bit];
}

static bool encode_bit(BIT_STATE *s, enum channel channel, enum bit bit, size_t max_samples, DATA_TYPE output[max_samples])
{
    const FREQ_TYPE frequency = get_frequency(channel, bit);
    const PHASE_STEP step = get_phase_step(frequency);

    size_t samples = (max_samples > SAMPLES_PER_BIT) ? SAMPLES_PER_BIT : max_samples;
    for (size_t i = 0; i < samples; i++) {
        encode_sample(&s->sample_state, step, &output[i]);
    }

    return true;
}

void encode_carrier(struct encode_state *s, size_t bit_times)
{
    int rc = 0;

    for (unsigned bit_index = 0; rc >= 0 && bit_index < bit_times; bit_index++) {
        DATA_TYPE output[SAMPLES_PER_BIT];
        rc = encode_bit(&s->bit_state, s->channel, BIT_ONE, SAMPLES_PER_BIT, output);
        s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
    }
}

void encode_bytes(struct encode_state *s, size_t byte_count, unsigned bytes[byte_count])
{
    int rc = 0;

    for (unsigned byte_index = 0; byte_index < byte_count; byte_index++) {
        DATA_TYPE output[SAMPLES_PER_BIT];
        unsigned byte = bytes[byte_index];

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.start_bits; bit_index++) {
            rc = encode_bit(&s->bit_state, s->channel, BIT_ZERO, SAMPLES_PER_BIT, output);
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            rc = encode_bit(&s->bit_state, s->channel, bit, SAMPLES_PER_BIT, output);
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        int oddness = POPCNT(byte) & 1;
        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.parity_bits; bit_index++) {
            // assume EVEN parity for now
            rc = encode_bit(&s->bit_state, s->channel, oddness, SAMPLES_PER_BIT, output);
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.stop_bits; bit_index++) {
            rc = encode_bit(&s->bit_state, s->channel, BIT_ONE, SAMPLES_PER_BIT, output);
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }
    }
}


