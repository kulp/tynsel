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

typedef struct sample_state SAMPLE_STATE;
typedef struct bit_state    BIT_STATE;
typedef struct byte_state   BYTE_STATE;

static const unsigned int SAMPLE_RATE = 8000;
static const unsigned int BAUD_RATE = 300;
static const unsigned int SAMPLES_PER_BIT = (SAMPLE_RATE + BAUD_RATE - 1) / BAUD_RATE; // round up (err on the slow side)

// returns whether a new byte was accepted (if `restart` was true)
typedef bool encode_pusher(const SERIAL_CONFIG *c, BYTE_STATE *s, bool restart, enum channel channel, uint8_t byte, void *out);

encode_pusher encode_bytes, encode_carrier;

#endif

