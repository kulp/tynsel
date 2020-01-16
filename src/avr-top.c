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
#include "sine.h"
#include "state.h"

// Work around in-progress support for atxmega3
#define SLEEP_CTRL SLPCTRL_CTRLA
#define SLEEP_SEN_bm SLPCTRL_SEN_bm

#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

static struct CONFIG {
    SERIAL_CONFIG serial;
    AUDIO_CONFIG audio;
} config EEMEM = {
    .serial = {
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
        .parity      = PARITY_SPACE,
    },
    .audio = {
        .channel     = CHAN_ZERO,
        .window_size = 6,
        .threshold   = 256,
        .hysteresis  = 10,
        .offset      = 12,
    },
};

// These flags will be tripped by interrupt handlers
volatile bool encoder_ready = false;
volatile bool decoder_ready = false;

// Serial in and out
volatile char serial_in  = 0;
volatile char serial_out = 0;

ISR(ADC0_RESRDY_vect)
{
    decoder_ready = true;
}

decode_pumper pump_decoder16;
encode_pusher encode_bytes16;
sines_init init_sines16;

static void init(BYTE_STATE *bs)
{
    DAC0.DATA = 0x7f; // half-scale output
    DAC0.CTRLA |= DAC_ENABLE_bm | DAC_OUTEN_bm;

    sines_init *init_sines = init_sines16;

    init_sines(&bs->bit_state.sample_state.quadrant, 1.0 /* ignored */);
}

static void run(BYTE_STATE *bs)
{
    decode_pumper *pump_decoder = pump_decoder16;
    encode_pusher *encode_bytes = encode_bytes16;

    while (true) {
        if (decoder_ready) {
            decoder_ready = false;

            char d = 0;
            DECODE_DATA_TYPE audio_in = (DECODE_DATA_TYPE)ADC0.RES;
            pump_decoder(&config.serial, &config.audio, &audio_in, &d);
            serial_out = d;
        }

        if (encoder_ready) {
            encoder_ready = false;

            ENCODE_DATA_TYPE e = 0;
            encode_bytes(&config.serial, bs, true, config.audio.channel, serial_in, &e);
            DAC0.DATA = (uint8_t)(e >> 8); // 16-bit data, 8-bit DAC
        }

        sleep_mode();
    }
}

int main()
{
    BYTE_STATE bs = { .channel = config.audio.channel };

    init(&bs);
    run(&bs);
}
