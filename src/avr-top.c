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

#define CONFIG_ATTRS volatile

#define CONFIG_LIST(_) \
    _(enum channel, channel    , CHAN_ZERO) \
    _(uint8_t     , window_size, 6        ) \
    _(RMS_OUT_DATA, threshold  , 256      ) \
    _(int8_t      , hysteresis , 10       ) \
    _(int8_t      , offset     , 12       ) \
    // end CONFIG_LIST

#define DECLARE_GLOBAL(Type, Name, Value) \
    CONFIG_ATTRS Type Name = Value;

CONFIG_LIST(DECLARE_GLOBAL)

CONFIG_ATTRS SERIAL_CONFIG cs = {
    .data_bits   = 7,
    .parity_bits = 1,
    .stop_bits   = 2,
    .parity      = PARITY_SPACE,
};

#define EXTERN_PTR(Type, Name) \
    ({ extern Type Name; &Name; })

// These flags will be tripped by interrupt handlers
volatile bool encoder_ready = false;
volatile bool decoder_ready = false;

// Serial in and out
volatile DECODE_OUT_DATA serial_in  = 0;
volatile DECODE_OUT_DATA serial_out = 0;

volatile DECODE_DATA_TYPE audio_in  = 0;
volatile ENCODE_DATA_TYPE audio_out = 0;

int main()
{
    BYTE_STATE bs = { .channel = channel };
    SERIAL_CONFIG c = cs;

#define DECLARE_LOCAL(Type, Name, Value) \
    Type Name = *EXTERN_PTR(CONFIG_ATTRS Type, Name);

    CONFIG_LIST(DECLARE_LOCAL)

    while (true) {
        if (decoder_ready) {
            decoder_ready = false;

            DECODE_OUT_DATA d = 0;
            pump_decoder(&c, channel, window_size, threshold, hysteresis, offset, audio_in, &d);
            serial_out = d;
        }

        if (encoder_ready) {
            encoder_ready = false;

            ENCODE_DATA_TYPE e = audio_out;
            encode_bytes(&c, &bs, true, channel, serial_in, &e);
        }

        __asm__("sleep");
    }
}

