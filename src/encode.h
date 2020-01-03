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

#ifndef ENCODE_H_
#define ENCODE_H_

#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DATA_TYPE int8_t
#define PHASE_TYPE uint16_t
#define PHASE_FRACTION_BITS 8

#define FREQ_TYPE uint16_t

#define TABLE_SIZE 64u
#define MAJOR_PER_CYCLE (TABLE_SIZE * 4)
#define MINOR_PER_CYCLE (MAJOR_PER_CYCLE * (1u << (PHASE_FRACTION_BITS)))

typedef struct {
    const DATA_TYPE (*quadrant)[TABLE_SIZE];
    PHASE_TYPE phase;
} SAMPLE_STATE;
typedef PHASE_TYPE PHASE_STEP;

typedef struct {
    SAMPLE_STATE sample_state;
    size_t samples_remaining;
    PHASE_STEP step;
    enum channel channel;
} BIT_STATE;

typedef struct {
    BIT_STATE bit_state;
    uint16_t current_word;
    uint16_t next_word;
    uint8_t bits_remaining;
    bool buffer_full;
} BYTE_STATE;

static const unsigned int SAMPLE_RATE = 8000;
static const unsigned int BAUD_RATE = 300;
static const size_t SAMPLES_PER_BIT = (SAMPLE_RATE + BAUD_RATE - 1) / BAUD_RATE; // round up (err on the slow side)

struct audio_state {
    int start_bits,
        data_bits,
        parity_bits,
        stop_bits;

    enum parity parity;
};

struct encode_state {
    struct audio_state audio;
    BYTE_STATE byte_state;
    int verbosity;
    enum channel channel;
    float gain;
    struct {
        void *userdata;
        int (*put_samples)(struct audio_state *a, size_t count, DATA_TYPE sample[count], void *userdata);
    } cb;
};

// returns number of samples emitted, or -1
bool encode_bytes(struct encode_state *s, bool restart, enum channel channel, uint8_t byte, DATA_TYPE *out);
bool encode_carrier(struct encode_state *s, bool restart, enum channel channel, DATA_TYPE *out);

#endif

