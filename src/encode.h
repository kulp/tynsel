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

#include "sine.h"
#include "types.h"

#include <stdbool.h>
#include <stdint.h>

#define PHASE_TYPE uint16_t
#define PHASE_FRACTION_BITS 8

#define FREQ_TYPE uint16_t

#define MINOR_PER_CYCLE (MAJOR_PER_CYCLE * (1u << (PHASE_FRACTION_BITS)))

typedef struct {
    const ENCODE_DATA_TYPE (*quadrant)[WAVE_TABLE_SIZE];
    PHASE_TYPE phase;
} SAMPLE_STATE;
typedef PHASE_TYPE PHASE_STEP;

typedef struct {
    SAMPLE_STATE sample_state;
    PHASE_STEP step;
    enum channel channel;
    uint8_t samples_remaining;
} BIT_STATE;

typedef struct {
    BIT_STATE bit_state;
    enum channel channel;
    uint16_t current_word;
    uint16_t next_word;
    uint8_t bits_remaining;
    bool buffer_full;
} BYTE_STATE;

static const unsigned int SAMPLE_RATE = 8000;
static const unsigned int BAUD_RATE = 300;
static const unsigned int SAMPLES_PER_BIT = (SAMPLE_RATE + BAUD_RATE - 1) / BAUD_RATE; // round up (err on the slow side)

// returns number of samples emitted, or -1
bool encode_bytes(const SERIAL_CONFIG *c, BYTE_STATE *s, bool restart, enum channel channel, uint8_t byte, ENCODE_DATA_TYPE *out);
bool encode_carrier(const SERIAL_CONFIG *c, BYTE_STATE *s, bool restart, enum channel channel, ENCODE_DATA_TYPE *out);

#endif

