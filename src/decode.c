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

#define BITWIDTH 27 /* 8000 / 300 */
#define THRESHOLD 0
#define MAX_RMS_SAMPLES 8

#if defined(__AVR__)
#define WARN(...) (void)(__VA_ARGS__)
#include <avr/pgmspace.h>
#else
#include <stdio.h>
#define WARN(Fmt,...) fprintf(stderr, Fmt "\n", ##__VA_ARGS__)
#define PROGMEM
#endif

struct bits_state {
    int8_t off;
    int8_t last;
    uint8_t bit;
    DECODE_OUT_DATA byte;
};

struct bits_config {
    uint8_t start_bits;
    uint8_t data_bits;
    uint8_t parity_bits;
    uint8_t stop_bits;
};

static bool decode(const struct bits_config *c, struct bits_state *s, int8_t offset, DECODE_IN_DATA datum, DECODE_OUT_DATA *out)
{
    const uint8_t before_parity = (uint8_t)(c->start_bits + c->data_bits);
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

            if (s->bit < c->start_bits) {
                // start bit(s)
                if (this_bit != 0) {
                    WARN("Start bit is not 0");
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else if (s->bit >= before_parity && s->bit < before_stop) {
                // parity, skip
            } else if (s->bit >= before_stop) {
                if (this_bit != 1) {
                    WARN("Stop bit was not 1");
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else {
                s->byte |= (DECODE_OUT_DATA)(this_bit << (s->bit - 1));
            }

            if (s->bit >= 10) {
                *out = s->byte;
                s->byte = 0;
                s->bit = 0;
                s->off = -1;
                return true;
            } else {
                s->bit++;
            }

            s->off = BITWIDTH;
        }
    } while (0);

    s->last = datum;
    s->off--;
    return false;
}

struct rms_state {
    RMS_OUT_DATA *window;
    RMS_OUT_DATA sum;
    uint8_t ptr;
    bool primed;
};

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

struct runs_state {
    RUNS_OUT_DATA current;
};

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

struct filter_config {
    FILTER_COEFF b[3], a[3];
};

struct filter_state {
    FILTER_IN_DATA in[3];
    FILTER_STATE_DATA out[3];
    uint8_t ptr;
};

static bool filter(const struct filter_config * PROGMEM c, struct filter_state *s, FILTER_IN_DATA datum, FILTER_OUT_DATA *out)
{
    s->in[s->ptr] = datum;

    // Avoid expensive modulo
    #define MOD(x,n) ((x) >= (n) ? (x) - (n) : (x))
    #define INDEX(x,n) (x)[MOD(s->ptr + (n) + 3, 3)]
#if !defined(__AVR__) || defined(__AVR_PM_BASE_ADDRESS__)
    #define COEFF(Type,Index) c->Type[Index]
#else
    #define COEFF(Type,Index) (FILTER_COEFF)pgm_read_word(&c->Type[Index])
#endif

    s->out[s->ptr] = 0
        + FILTER_MULT(COEFF(b, 0), INDEX(s->in ,  0))
        + FILTER_MULT(COEFF(b, 1), INDEX(s->in , -1))
        + FILTER_MULT(COEFF(b, 2), INDEX(s->in , -2))

        // coefficient a0 is special, and does not appear here
        - FILTER_MULT(COEFF(a, 1), INDEX(s->out, -1))
        - FILTER_MULT(COEFF(a, 2), INDEX(s->out, -2))
        ;

    *out = (FILTER_OUT_DATA)s->out[s->ptr];

    ++s->ptr;
    s->ptr = (uint8_t)MOD(s->ptr, 3);

    return true;
}

static const struct filter_config coeffs[CHAN_max][BIT_max] PROGMEM = {
    [CHAN_ZERO][BIT_ZERO] =
            #include "coeffs_1070_8000_150.h"
        ,
    [CHAN_ZERO][BIT_ONE] =
            #include "coeffs_1270_8000_150.h"
        ,
    [CHAN_ONE][BIT_ZERO] =
            #include "coeffs_2025_8000_150.h"
        ,
    [CHAN_ONE][BIT_ONE] =
            #include "coeffs_2225_8000_150.h"
        ,
};

bool pump_decoder(
        enum channel channel,
        uint8_t window_size,
        uint16_t threshold,
        int8_t hysteresis,
        int8_t offset,
        FILTER_IN_DATA in,
        DECODE_OUT_DATA *out
    )
{
    static RMS_OUT_DATA large[2][MAX_RMS_SAMPLES] = { { 0 } };
    static struct rms_state rms_states[2] = {
        { .window = large[0] },
        { .window = large[1] },
    };

    static struct filter_state filt_states[2] = { { .ptr = 0 } };

    static struct runs_state run_state = { .current = 0 };

    static struct bits_state dec_state = { .off = -1, .last = THRESHOLD };
    static const struct bits_config dec_conf = {
        .start_bits  = 1,
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };

    static FILTER_OUT_DATA f[2] = { 0 };
    if (
            ! filter(&coeffs[channel][BIT_ZERO], &filt_states[0], in, &f[0])
        ||  ! filter(&coeffs[channel][BIT_ONE ], &filt_states[1], in, &f[1])
        )
        return false;

    static RMS_OUT_DATA ra = 0, rb = 0;
    if (
            ! rms(window_size, &rms_states[0], f[0] - in, &ra)
        ||  ! rms(window_size, &rms_states[1], f[1] - in, &rb)
        )
        return false;

    if (ra < threshold && rb < threshold)
        return false;

    static RUNS_OUT_DATA ro = 0;
    if (! runs(hysteresis, &run_state, ra, rb, &ro))
        return false;

    return decode(&dec_conf, &dec_state, offset, ro, out);
}

