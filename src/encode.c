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

static bool encode_bit(BIT_STATE *s, bool newbit, enum channel channel, enum bit bit, DATA_TYPE *out)
{
    const FREQ_TYPE frequency = get_frequency(channel, bit);
    const PHASE_STEP step = get_phase_step(frequency);

    (void)newbit;
    return encode_sample(&s->sample_state, step, out);
}

bool push_raw_word(BYTE_STATE *s, bool restart, enum channel channel, uint16_t word, DATA_TYPE *out)
{
    bool busy = s->bits_remaining > 0;
    bool full = s->buffer_full;

    bool newbit = false;
    enum bit bit = 1; // idle bit

    #define BOOL_TRIAD(A,B,C) (((A) << 2) | ((B) << 1) | ((C) << 0))
    switch (BOOL_TRIAD(restart, full, busy)) {
        case BOOL_TRIAD(true , true , false): // full, need to fill and emit
            s->current_word = s->next_word;
            s->buffer_full = false;
            // FALLTHROUGH
        case BOOL_TRIAD(true , false, true ): // emitting, need to fill
        case BOOL_TRIAD(true , false, false): // idle, need to fill
            s->next_word = word;
            s->buffer_full = true;
            return push_raw_word(s, restart, channel, word, out); // tail recursion

        case BOOL_TRIAD(false, true , false): // full, need to emit
            s->current_word = s->next_word;
            s->buffer_full = false;
            // FALLTHROUGH
        case BOOL_TRIAD(false, true , true ): // full, emitting
        case BOOL_TRIAD(false, false, true ): // emitting
            bit = s->current_word & 1;
            s->current_word >>= 1;
            s->bits_remaining--;
            // FALLTHROUGH
        case BOOL_TRIAD(false, false, false): // idle
        case BOOL_TRIAD(true , true , true ): // full, emitting, no room
            return encode_bit(&s->bit_state, newbit, channel, bit, out);
    }

    return false; // this is meant to be unreachable
}

void encode_carrier(struct encode_state *s, size_t bit_times)
{
    int rc = 0;

    for (unsigned bit_index = 0; rc >= 0 && bit_index < bit_times; bit_index++) {
        DATA_TYPE output[SAMPLES_PER_BIT];
        for (DATA_TYPE *out = output; out < &output[SAMPLES_PER_BIT]; out++)
            while (! encode_bit(&s->bit_state, true, s->channel, BIT_ONE, out))
                ; // spin
        s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
    }
}

static inline bool compute_parity(enum parity parity, uint8_t byte)
{
    bool oddness = POPCNT(byte) & 1;
    switch (parity) {
        case PARITY_SPACE: return false;
        case PARITY_MARK : return true;
        case PARITY_EVEN : return  oddness; // make the number of bits even
        case PARITY_ODD  : return ~oddness; // keep the number of bits odd
    }

    return false; // this is meant to be unreachable
}

void encode_bytes(struct encode_state *s, size_t byte_count, unsigned bytes[byte_count])
{
    int rc = 0;

    for (unsigned byte_index = 0; byte_index < byte_count; byte_index++) {
        DATA_TYPE output[SAMPLES_PER_BIT];
        unsigned byte = bytes[byte_index];

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.start_bits; bit_index++) {
            for (DATA_TYPE *out = output; out < &output[SAMPLES_PER_BIT]; out++)
                while (! encode_bit(&s->bit_state, true, s->channel, BIT_ZERO, out))
                    ; // spin
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            for (DATA_TYPE *out = output; out < &output[SAMPLES_PER_BIT]; out++)
                while (! encode_bit(&s->bit_state, true, s->channel, bit, out))
                    ; // spin
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        bool parity = compute_parity(s->audio.parity, byte);
        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.parity_bits; bit_index++) {
            for (DATA_TYPE *out = output; out < &output[SAMPLES_PER_BIT]; out++)
                while (! encode_bit(&s->bit_state, true, s->channel, parity, out))
                    ; // spin
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }

        for (int bit_index = 0; rc >= 0 && bit_index < s->audio.stop_bits; bit_index++) {
            for (DATA_TYPE *out = output; out < &output[SAMPLES_PER_BIT]; out++)
                while (! encode_bit(&s->bit_state, true, s->channel, BIT_ONE, out))
                    ; // spin
            s->cb.put_samples(&s->audio, SAMPLES_PER_BIT, output, s->cb.userdata);
        }
    }
}


