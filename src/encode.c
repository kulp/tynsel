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
#include "sine.h"
#include "state.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define MINOR_PER_CYCLE ((WAVE_TABLE_SIZE * 4) * (1u << (PHASE_FRACTION_BITS)))

typedef ENCODE_DATA_TYPE DATA_TYPE;

#define TEST_BIT(Word,Index) (((Word) & (1 << (Index))) ? -1 : 0)

static void encode_sample(SAMPLE_STATE *s, PHASE_STEP step, DATA_TYPE *out)
{
    uint8_t top = (uint8_t)(s->phase >> PHASE_FRACTION_BITS);
    DATA_TYPE  half    = (DATA_TYPE )TEST_BIT(top, 7);
    PHASE_TYPE quarter = (PHASE_TYPE)TEST_BIT(top, 6);
    uint8_t lookup = (top ^ quarter) % WAVE_TABLE_SIZE;

    s->phase += step;

    *out = ((ENCODE_DATA_TYPE*)s->quadrant)[lookup] ^ half;
}

static inline PHASE_STEP get_phase_step(uint16_t freq)
{
    return (PHASE_STEP)(MINOR_PER_CYCLE * freq / SAMPLE_RATE);
}

static inline uint16_t get_frequency(enum channel channel, enum bit bit)
{
    #define INIT_2D_ARRAY(X,Y,Z) [X][Y] = Z,

    static const uint16_t freqs[CHAN_max][BIT_max] = { FREQUENCY_LIST(INIT_2D_ARRAY) };

    return freqs[channel][bit];
}

static bool encode_bit(BIT_STATE *s, bool newbit, enum channel channel, enum bit bit, DATA_TYPE *out)
{
    bool busy = s->samples_remaining > 0;

    if (! busy) {
        s->samples_remaining = SAMPLES_PER_BIT;
        if (newbit) {
            s->channel = channel;
        } else {
            // if no new bit, switch back to the idle bit in the most-recent channel
            bit = BIT_ONE;
        }
        s->step = get_phase_step(get_frequency(s->channel, bit));
    }

    if (s->samples_remaining) {
        encode_sample(&s->sample_state, s->step, out);
        s->samples_remaining--;
    }

    return ! busy;
}

static bool push_raw_word(BYTE_STATE *s, bool restart, enum channel channel, uint8_t bit_count, uint16_t word, DATA_TYPE *out)
{
    bool busy = s->bits_remaining > 0;
    bool full = s->buffer_full;

    enum bit bit = (enum bit)(s->current_word & 1);

    #define BOOL_TRIAD(A,B,C) (((A) << 2) | ((B) << 1) | ((C) << 0))
    switch (BOOL_TRIAD(restart, full, busy)) {
        case BOOL_TRIAD(true , true , false): // full, need to fill and emit
            s->current_word = s->next_word;
            s->buffer_full = false;
            s->bits_remaining = bit_count;
            // FALLTHROUGH
        case BOOL_TRIAD(true , false, true ): // emitting, need to fill
        case BOOL_TRIAD(true , false, false): // idle, need to fill
            s->next_word = word;
            s->buffer_full = true;
            return push_raw_word(s, false, channel, bit_count, word, out); // tail recursion

        case BOOL_TRIAD(true , true , true ): // full, emitting, no room
            if (encode_bit(&s->bit_state, busy, channel, bit, out)) {
                s->current_word >>= 1;
                s->bits_remaining--;
            }
            return false;

        case BOOL_TRIAD(false, false, false): // idle
            return encode_bit(&s->bit_state, busy, channel, bit, out);

        case BOOL_TRIAD(false, true , false): // full, need to emit
            s->current_word = s->next_word;
            s->buffer_full = false;
            s->bits_remaining = bit_count;
            busy = true;
            // FALLTHROUGH
        case BOOL_TRIAD(false, true , true ): // full, emitting
        case BOOL_TRIAD(false, false, true ): // emitting
            if (encode_bit(&s->bit_state, busy, channel, bit, out)) {
                s->current_word >>= 1;
                s->bits_remaining--;
            }
            return true;
    }

    return false; // this is meant to be unreachable
}

static inline uint8_t count_bits(const SERIAL_CONFIG *s)
{
    return (uint8_t)(NUM_START_BITS + s->data_bits + s->parity_bits + s->stop_bits);
}

bool CAT(encode_carrier,ENCODE_BITS)(const SERIAL_CONFIG *c, BYTE_STATE *s, bool restart, enum channel channel, char byte, void *out)
{
    (void)byte; // unused
    return push_raw_word(s, restart, channel, count_bits(c), (uint16_t)-1u, (DATA_TYPE*)out);
}

static inline uint8_t popcnt(char x)
{
#if defined(__GNUC__)
    return (uint8_t)__builtin_popcount((unsigned int)x);
#else
    uint8_t out = 0;
    while (x >>= 1)
        out = (uint8_t)(out + (x & 1u));
    return out;
#endif
}

static inline bool compute_parity(enum parity parity, char byte)
{
    bool oddness = popcnt(byte) & 1;
    switch (parity) {
        case PARITY_SPACE: return false;
        case PARITY_MARK : return true;
        case PARITY_EVEN : return   oddness; // make the number of bits even
        case PARITY_ODD  : return ! oddness; // keep the number of bits odd
    }

    return false; // this is meant to be unreachable
}

static inline uint16_t make_word(const SERIAL_CONFIG *s, char byte)
{
    #define MASK(Width) ((1u << (Width)) - 1)
    uint16_t word = 0;

    word |= MASK(s->stop_bits) & -1u;

    word <<= s->parity_bits;
    word |= MASK(s->parity_bits) & compute_parity(s->parity, byte);

    word <<= s->data_bits;
    word |= MASK(s->data_bits) & (uint8_t)byte;

    word <<= NUM_START_BITS;
    word |= MASK(NUM_START_BITS) & 0;

    return word;
}

bool CAT(encode_bytes,ENCODE_BITS)(const SERIAL_CONFIG *c, BYTE_STATE *s, bool restart, enum channel channel, char byte, void *out)
{
    uint8_t bit_count = count_bits(c);
    uint16_t word = make_word(c, byte);
    return push_raw_word(s, restart, channel, bit_count, word, (DATA_TYPE*)out);
}

