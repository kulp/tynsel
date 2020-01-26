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
typedef struct {
    bool encoder_ready:1;
    bool decoder_ready:1;
    unsigned :6;
} INTERRUPT_FLAGS;
static volatile INTERRUPT_FLAGS *flags = (volatile INTERRUPT_FLAGS *)&GPIOR0;

// Serial in and out
volatile char serial_in  = 0;
volatile char serial_out = 0;

ISR(ADC0_RESRDY_vect)
{
    flags->decoder_ready = true;
}

EXTERN decode_init CAT(decode_state_init,DECODE_BITS);
EXTERN decode_pumper CAT(pump_decoder,DECODE_BITS);
EXTERN decode_fini CAT(decode_state_fini,DECODE_BITS);

EXTERN encode_pusher CAT(encode_bytes,ENCODE_BITS);
EXTERN sines_init CAT(init_sines,ENCODE_BITS);

static void init(BYTE_STATE *bs, DECODE_STATE **ds)
{
    DAC0.DATA = 0x7f; // half-scale output
    DAC0.CTRLA |= DAC_ENABLE_bm | DAC_OUTEN_bm;

    sines_init *init_sines = CAT(init_sines,ENCODE_BITS);

    init_sines(&bs->bit_state.sample_state.quadrant, 1.0 /* ignored */);

    decode_init *init_decoder = CAT(decode_state_init,DECODE_BITS);
    *ds = init_decoder();
}

_Noreturn static void run(BYTE_STATE *bs, DECODE_STATE *ds)
{
    decode_pumper *pump_decoder = CAT(pump_decoder,DECODE_BITS);
    encode_pusher *encode_bytes = CAT(encode_bytes,ENCODE_BITS);

    while (true) {
        if (flags->decoder_ready) {
            flags->decoder_ready = false;

            char d = 0;
            DECODE_DATA_TYPE audio_in = (DECODE_DATA_TYPE)ADC0.RES;
            pump_decoder(&tt_serial, &audio, &coeff_table[audio.channel], ds, &audio_in, &d);
            serial_out = d;
        }

        if (flags->encoder_ready) {
            flags->encoder_ready = false;

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
    decode_fini *fini_decoder = CAT(decode_state_fini,DECODE_BITS);
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
