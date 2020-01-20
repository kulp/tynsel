/*
 * Copyright (c) 2019-2020 Darren Kulp
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

#include "types.h"

#include <stdbool.h>
#include <stdint.h>

#define DECODE_DATA_TYPE SIZED(DECODE_BITS)

typedef uint16_t RMS_OUT_DATA;

struct filter_config;

typedef struct {
    enum channel channel;
    uint8_t      window_size;
    RMS_OUT_DATA threshold;
    int8_t       hysteresis;
    int8_t       offset;
} AUDIO_CONFIG;

typedef struct decode_state DECODE_STATE;

typedef DECODE_STATE *decode_init();
typedef void decode_fini(DECODE_STATE *s);

typedef bool decode_pumper(
        const SERIAL_CONFIG *config,
        const AUDIO_CONFIG *audio,
        const struct filter_config *coeffs,
        DECODE_STATE *s,
        void *in,
        char *out
    );

#endif

