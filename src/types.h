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

#define ENCODE_DATA_TYPE int16_t
#define DECODE_DATA_TYPE int16_t

enum channel { CHAN_ZERO, CHAN_ONE, CHAN_max };
enum bit { BIT_ZERO, BIT_ONE, BIT_max };
enum parity { PARITY_SPACE, PARITY_MARK, PARITY_EVEN, PARITY_ODD };

typedef struct {
    uint8_t start_bits;
    uint8_t data_bits;
    uint8_t parity_bits;
    uint8_t stop_bits;

    enum parity parity;
} SERIAL_CONFIG;

#endif
