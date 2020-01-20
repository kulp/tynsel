/*
 * Copyright (c) 2020 Darren Kulp
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

#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>

#define STR(X) STR_(X)
#define STR_(X) # X

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

#define TYPE(Prefix, Bits, Suffix) \
    CAT(Prefix,CAT(Bits,Suffix))

#define SIZED(Bits) TYPE(int, Bits, _t)

enum channel { CHAN_ZERO, CHAN_ONE, CHAN_max };
enum bit { BIT_ZERO, BIT_ONE, BIT_max };
enum parity { PARITY_SPACE, PARITY_MARK, PARITY_EVEN, PARITY_ODD };

typedef struct {
    uint8_t data_bits;
    uint8_t parity_bits;
    uint8_t stop_bits;

    enum parity parity;
} SERIAL_CONFIG;

enum { NUM_START_BITS = 1 };

#ifndef SAMPLE_RATE
#error "#define SAMPLE_RATE in Hz"
#endif

static const unsigned int BAUD_RATE = 300;
static const unsigned int SAMPLES_PER_BIT = (SAMPLE_RATE + BAUD_RATE - 1) / BAUD_RATE; // round up (err on the slow side)

#define FREQUENCY_LIST(_) \
    _(CHAN_ZERO, BIT_ZERO, 1070) \
    _(CHAN_ZERO, BIT_ONE , 1270) \
    _(CHAN_ONE , BIT_ZERO, 2025) \
    _(CHAN_ONE , BIT_ONE , 2225) \
    // end macro

#endif
