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

#include "decode.h"
#include "encode.h"

volatile DECODE_DATA_TYPE d_in = 0;
volatile DECODE_OUT_DATA d_out = 0;
volatile ENCODE_DATA_TYPE e_out = 0;

int __attribute__((used)) main()
{
    const enum channel channel = CHAN_ZERO;
    const uint8_t window_size = 6;
    const uint16_t threshold = 256;
    const int8_t hysteresis = 10;
    const int8_t offset = 12;

    SERIAL_CONFIG cs = {
        .start_bits  = 1,
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
        .parity      = PARITY_SPACE,
    };
    BYTE_STATE bs = { .channel = channel };

    while (true) {
        // TODO make a real implementation : this one serves simply to ensure that
        // functions are seen by the compiler to be used
        DECODE_OUT_DATA d = 0;
        pump_decoder(channel, window_size, threshold, hysteresis, offset, d_in, &d);
        d_out = d;

        ENCODE_DATA_TYPE e = e_out;
        encode_bytes(&cs, &bs, true, channel, 'K', &e);
    }
}

