/*
 * Copyright (c) 2012 Darren Kulp
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

#ifndef DECODE_H_
#define DECODE_H_

#include "audio.h"
#include <stddef.h>

struct decode_state {
    struct audio_state audio;
    int verbosity;
};

typedef int bit_recogniser(struct decode_state *s, size_t size, double samples[size], int *channel, int *bit, float *probability);

int decode_byte(struct decode_state *s, size_t size, double input[size], int output[], double *offset, int channel);
int decode_data(struct decode_state *s, size_t count, double input[count]);
void decode_cleanup(void);

#define SAMPLES_PER_BIT(s) ((double)s->audio.sample_rate / s->audio.baud_rate)

#endif

