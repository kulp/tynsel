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

#include "coeff.h"
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

static SERIAL_CONFIG tt_serial EEMEM = {
    .data_bits   = 7,
    .parity_bits = 1,
    .stop_bits   = 2,
    .parity      = PARITY_SPACE,
};

static SERIAL_CONFIG host_serial EEMEM __attribute__((used)) = {
    .data_bits   = 8,
    .parity_bits = 0,
    .stop_bits   = 1,
};

static AUDIO_CONFIG audio EEMEM = {
    .channel     = CHAN_ZERO,
    .window_size = 6,
    .threshold   = 256,
    .hysteresis  = 10,
    .offset      = 12,
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

decode_init decode_state_init16;
decode_pumper pump_decoder16;
decode_fini decode_state_fini16;

encode_pusher encode_bytes16;
sines_init init_sines8;

static void init(BYTE_STATE *bs, DECODE_STATE **ds)
{
    DAC0.DATA = 0x7f; // half-scale output
    DAC0.CTRLA |= DAC_ENABLE_bm | DAC_OUTEN_bm;

    sines_init *init_sines = init_sines8;

    init_sines(&bs->bit_state.sample_state.quadrant, 1.0 /* ignored */);

    decode_init *init_decoder = decode_state_init16;
    *ds = init_decoder();
}

_Noreturn static void run(BYTE_STATE *bs, DECODE_STATE *ds)
{
    decode_pumper *pump_decoder = pump_decoder16;
    encode_pusher *encode_bytes = encode_bytes16;

    while (true) {
        if (decoder_ready) {
            decoder_ready = false;

            char d = 0;
            DECODE_DATA_TYPE audio_in = (DECODE_DATA_TYPE)ADC0.RES;
            pump_decoder(&tt_serial, &audio, &coeff_table[audio.channel], ds, &audio_in, &d);
            serial_out = d;
        }

        if (encoder_ready) {
            encoder_ready = false;

            ENCODE_DATA_TYPE e = 0;
            encode_bytes(&tt_serial, bs, true, audio.channel, serial_in, &e);
            DAC0.DATA = (register8_t)e;
        }

        sleep_mode();
    }
}

static void fini(BYTE_STATE *bs, DECODE_STATE **ds)
{
    (void)bs;
    decode_fini *fini_decoder = decode_state_fini16;
    fini_decoder(*ds);
    *ds = NULL;
}

int main()
{
    BYTE_STATE bs = { .channel = audio.channel };
    DECODE_STATE *ds = NULL;

    init(&bs, &ds);
    run(&bs, ds);
    fini(&bs, &ds);
}
