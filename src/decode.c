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

#include "decode.h"
#include "decode-impl.h"

#include <assert.h>
#include <limits.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#include <stdio.h>
#define PROGMEM
#endif

static bool decode(const SERIAL_CONFIG *c, struct bits_state *s, int8_t offset, DECODE_IN_DATA datum, char *out)
{
    const uint8_t before_parity = (uint8_t)(NUM_START_BITS + c->data_bits);
    const uint8_t before_stop   = (uint8_t)(before_parity + c->parity_bits);
    do {
        if (s->bit == 0 && datum >= THRESHOLD && s->last < THRESHOLD) {
            s->off = offset;
        }

        if (s->off < 0)
            break;

        if (s->off == 0) {
            // sample here
            uint8_t this_bit = datum < 0;

            if (s->bit < NUM_START_BITS) {
                // start bit(s)
                if (this_bit != 0) {
                    // Start bit is not 0 -- restart
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else if (s->bit >= before_parity && s->bit < before_stop) {
                // parity, skip
            } else if (s->bit >= before_stop) {
                if (this_bit != 1) {
                    // Stop bit was not 1 -- abort this byte
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else {
                s->byte |= (char)(this_bit << (s->bit - 1));
            }

            if (s->bit >= before_stop + 1) { // accept a minimum number of stop bits
                *out = s->byte;
                s->byte = 0;
                s->bit = 0;
                s->off = -1;
                return true;
            } else {
                s->bit++;
            }

            s->off = SAMPLES_PER_BIT;
        }
    } while (0);

    s->last = datum;
    s->off--;
    return false;
}

// TODO rename -- we do not actually do the "root" part of RMS since it is
// expensive and for our purposes unnecessary.
static bool rms(const uint8_t window_size, struct rms_state *s, RMS_IN_DATA datum, RMS_OUT_DATA *out)
{
    s->sum -= s->window[s->ptr];
    s->window[s->ptr] = (RMS_OUT_DATA)(datum * datum);
    s->sum += s->window[s->ptr];

    if (s->ptr == window_size - 1)
        s->primed = true;

    // Avoid expensive modulo
    if (++s->ptr >= window_size)
        s->ptr = 0;

    if (s->primed)
        *out = s->sum;

    return s->primed;
}

static bool runs(int8_t hysteresis, struct runs_state *s, RUNS_IN_DATA da, RUNS_IN_DATA db, RUNS_OUT_DATA *out)
{
    int8_t inc = (da > db) ?  1 :
                 (da < db) ? -1 :
                              0 ;
    s->current = (RUNS_OUT_DATA)(s->current + inc);

    const int8_t max = (int8_t) hysteresis;
    const int8_t min = (int8_t)-hysteresis;

    if (s->current > max)
        s->current = max;

    if (s->current < min)
        s->current = min;

    *out = s->current;

    return true;
}

static bool filter(const struct filter_config * PROGMEM c, struct filter_state *s, FILTER_IN_DATA datum, FILTER_OUT_DATA *out)
{
    s->in[s->ptr] = datum;

    // Avoid expensive modulo
    #define MOD(x,n) ((x) >= (n) ? (x) - (n) : (x))
    #define INDEX(x,n) (x)[MOD(s->ptr + (n) + 3, 3)]
    #define RAW_COEFF(Type,Index) c->coeff_##Type##Index
#if ! defined(__AVR__) || defined(__AVR_PM_BASE_ADDRESS__)
    #define COEFF(Type,Index) RAW_COEFF(Type,Index)
#else
    #define COEFF(Type,Index) (FILTER_COEFF)pgm_read_word(&RAW_COEFF(Type,Index))
#endif

    s->out[s->ptr] = 0
        + FILTER_MULT(COEFF(b, 0), EXPAND(INDEX(s->in,  0), FILTER_STATE_DATA))
        + FILTER_MULT(COEFF(b, 1), EXPAND(INDEX(s->in, -1), FILTER_STATE_DATA))
        + FILTER_MULT(COEFF(b, 2), EXPAND(INDEX(s->in, -2), FILTER_STATE_DATA))

        // coefficient a0 is special, and does not appear here
        - FILTER_MULT(COEFF(a, 1), INDEX(s->out, -1))
        - FILTER_MULT(COEFF(a, 2), INDEX(s->out, -2))
        ;

    *out = (FILTER_OUT_DATA)SHRINK(s->out[s->ptr], FILTER_OUT_DATA);

    ++s->ptr;
    s->ptr = (uint8_t)MOD(s->ptr, 3);

    return true;
}

bool CAT(pump_decoder,DECODE_BITS)(
        const SERIAL_CONFIG *c,
        const AUDIO_CONFIG *audio,
        const struct filter_config *coeffs,
        DECODE_STATE *s,
        void *p,
        char *out
    )
{
    DECODE_DATA_TYPE *in = (DECODE_DATA_TYPE*)p;

    FILTER_OUT_DATA f[2] = { 0 };
    if (
            ! filter(&coeffs[BIT_ZERO], &s->filt[0], *in, &f[0])
        ||  ! filter(&coeffs[BIT_ONE ], &s->filt[1], *in, &f[1])
        )
        return false;

    RMS_OUT_DATA ra = 0, rb = 0;
    if (
            ! rms(audio->window_size, &s->rms[0], (int8_t)SHRINK((FILTER_OUT_DATA)(f[0] - *in), int8_t), &ra)
        ||  ! rms(audio->window_size, &s->rms[1], (int8_t)SHRINK((FILTER_OUT_DATA)(f[1] - *in), int8_t), &rb)
        )
        return false;

    if (ra < audio->threshold && rb < audio->threshold)
        return false;

    RUNS_OUT_DATA ro = 0;
    if (! runs(audio->hysteresis, &s->run, ra, rb, &ro))
        return false;

    return decode(c, &s->dec, audio->offset, ro, out);
}

