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

#include <stddef.h>

enum channel { CHAN_ZERO, CHAN_ONE, CHAN_max };
enum bit { BIT_ZERO, BIT_ONE, BIT_max };

struct audio_state {
    unsigned sample_rate;
    double sample_offset;
    const unsigned baud_rate;

    int start_bits,
        data_bits,
        parity_bits,
        stop_bits;
};

struct encode_state {
    struct audio_state audio;
    int verbosity;
    unsigned channel;
    float gain;
    struct {
        void *userdata;
        int (*put_samples)(struct audio_state *a, size_t count, double sample[count], void *userdata);
    } cb;
};

// returns number of samples emitted, or -1
int encode_bytes(struct encode_state *s, size_t byte_count, unsigned bytes[byte_count]);
int encode_carrier(struct encode_state *s, size_t bit_times);

#endif

