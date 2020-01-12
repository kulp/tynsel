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

#ifndef STATE_H_
#define STATE_H_

#include "encode.h"

#include <stdint.h>

#define PHASE_FRACTION_BITS 8
#define PHASE_TYPE uint16_t
typedef PHASE_TYPE PHASE_STEP;

struct sample_state {
    const void *quadrant;
    PHASE_TYPE phase;
};

struct bit_state {
    SAMPLE_STATE sample_state;
    PHASE_STEP step;
    enum channel channel;
    uint8_t samples_remaining;
};

struct byte_state {
    BIT_STATE bit_state;
    enum channel channel;
    uint16_t current_word;
    uint16_t next_word;
    uint8_t bits_remaining;
    bool buffer_full;
};

#endif

