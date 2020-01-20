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

#ifndef DECODE_IMPL_H_
#define DECODE_IMPL_H_

#include "coeff.h"
#include "decode.h"

#define THRESHOLD 0
#define MAX_RMS_SAMPLES 8

#define EXPAND(X,Y) (assert(sizeof(Y) >= sizeof(X)), (X) << (CHAR_BIT * (sizeof(Y) - sizeof(X))))
#define SHRINK(X,Y) (assert(sizeof(X) >= sizeof(Y)), (X) >> (CHAR_BIT * (sizeof(X) - sizeof(Y))))

typedef DECODE_DATA_TYPE FILTER_IN_DATA;
typedef FILTER_IN_DATA FILTER_OUT_DATA;

typedef int8_t RMS_IN_DATA;
typedef int8_t RUNS_OUT_DATA;

typedef RMS_OUT_DATA RUNS_IN_DATA;
typedef RUNS_OUT_DATA DECODE_IN_DATA;

struct bits_state {
    int8_t off;
    int8_t last;
    uint8_t bit;
    char byte;
};

struct rms_state {
    RMS_OUT_DATA window[MAX_RMS_SAMPLES];
    RMS_OUT_DATA sum;
    uint8_t ptr;
    bool primed;
};

struct runs_state {
    RUNS_OUT_DATA current;
};

struct filter_state {
    FILTER_STATE_DATA in[3];
    FILTER_STATE_DATA out[3];
    uint8_t ptr;
};

struct decode_state {
    struct rms_state rms[2];
    struct filter_state filt[2];
    struct runs_state run;
    struct bits_state dec;
};

#endif

