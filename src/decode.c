#include <stdbool.h>
#include <stdint.h>

#define BITWIDTH 27 /* 8000 / 300 */
#define THRESHOLD 0
#define RMS_THRESHOLD 50000
#define MAX_RMS_SAMPLES 7 // empirically-determined sweet-spot

#define COEFF_FRACTIONAL_BITS 14

#if 0
typedef float FILTER_COEFF;
typedef float FILTER_STATE_DATA;
#define FLOAT_TO_COEFF(x) (x)
#define FILTER_MULT(a, b) ((a) * (b))
#else
typedef int16_t FILTER_COEFF;
typedef int16_t FILTER_STATE_DATA;
#define FLOAT_TO_COEFF(x) ((FILTER_COEFF)((x) * (1 << COEFF_FRACTIONAL_BITS)))
#define FILTER_MULT(a, b) (((a) * (b)) >> COEFF_FRACTIONAL_BITS)
#endif

#define FILTER_COEFF_read(Arg) FLOAT_TO_COEFF(strtof((Arg), NULL))

typedef int16_t FILTER_IN_DATA;
typedef FILTER_IN_DATA FILTER_OUT_DATA;

#ifdef __AVR__
typedef int16_t RMS_IN_DATA;
// Consider using __uint24 for RMS_OUT_DATA if possible (code size optimization)
typedef uint32_t RMS_OUT_DATA;
typedef int8_t RUNS_OUT_DATA;
typedef uint8_t DECODE_OUT_DATA;
#else
typedef int16_t RMS_IN_DATA;
typedef uint32_t RMS_OUT_DATA;
typedef int8_t RUNS_OUT_DATA;
typedef uint8_t DECODE_OUT_DATA;
#endif

typedef RMS_OUT_DATA RUNS_IN_DATA;
typedef RUNS_OUT_DATA DECODE_IN_DATA;

#ifdef __AVR__
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

static bool runs(int8_t threshold, struct runs_state *s, RUNS_IN_DATA da, RUNS_IN_DATA db, RUNS_OUT_DATA *out)
{
    int8_t inc = (da > db) ?  1 :
                 (da < db) ? -1 :
                              0 ;
    s->current = (RUNS_OUT_DATA)(s->current + inc);

    const int8_t max = (int8_t) threshold;
    const int8_t min = (int8_t)-threshold;

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

enum channel { CHAN_ZERO, CHAN_ONE, CHAN_max };
enum bit { BIT_ZERO, BIT_ONE, BIT_max };

static const struct filter_config coeffs[CHAN_max][BIT_max] PROGMEM = {
    [CHAN_ZERO] = {
        [BIT_ZERO] = {   // pei_tseng_notch(1070/(8000/2),150/(8000/2))
            .b = { FLOAT_TO_COEFF(+0.942850f), FLOAT_TO_COEFF(-1.258100f), FLOAT_TO_COEFF(+0.942850f) },
            .a = { FLOAT_TO_COEFF(+1.000000f), FLOAT_TO_COEFF(-1.258100f), FLOAT_TO_COEFF(+0.885690f) },
        },

        [BIT_ONE] = {   // pei_tseng_notch(1270/(8000/2),150/(8000/2))
            .b = { FLOAT_TO_COEFF(+0.943270f), FLOAT_TO_COEFF(-1.023340f), FLOAT_TO_COEFF(+0.943270f) },
            .a = { FLOAT_TO_COEFF(+1.000000f), FLOAT_TO_COEFF(-1.023340f), FLOAT_TO_COEFF(+0.886540f) },
        },
    },
    [CHAN_ONE] = {
        [BIT_ZERO] = {   // pei_tseng_notch(2025/(8000/2),150/(8000/2))
            .b = { FLOAT_TO_COEFF(+0.944342f), FLOAT_TO_COEFF(+0.037082f), FLOAT_TO_COEFF(+0.944342f) },
            .a = { FLOAT_TO_COEFF(+1.000000f), FLOAT_TO_COEFF(+0.037082f), FLOAT_TO_COEFF(+0.888683f) },
        },

        [BIT_ONE] = {   // pei_tseng_notch(2225/(8000/2),150/(8000/2))
            .b = { FLOAT_TO_COEFF(+0.944590f), FLOAT_TO_COEFF(+0.332110f), FLOAT_TO_COEFF(+0.944590f) },
            .a = { FLOAT_TO_COEFF(+1.000000f), FLOAT_TO_COEFF(+0.332110f), FLOAT_TO_COEFF(+0.889170f) },
        },
    },
};

bool top(
        uint8_t window_size,
        int8_t threshold,
        int8_t offset,
        FILTER_IN_DATA in,
        DECODE_OUT_DATA *out
    )
{
    static RMS_OUT_DATA large[2][MAX_RMS_SAMPLES] = { };
    static struct rms_state rms_states[2] = {
        { .window = large[0] },
        { .window = large[1] },
    };

    static struct filter_state filt_states[2] = { };

    static struct runs_state run_state = { .current = 0 };

    static struct bits_state dec_state = { .off = -1, .last = THRESHOLD };
    static const struct bits_config dec_conf = {
        .start_bits  = 1,
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };

    FILTER_OUT_DATA f[2] = { };
    if (
            ! filter(&coeffs[CHAN_ZERO][BIT_ZERO], &filt_states[0], in, &f[0])
        ||  ! filter(&coeffs[CHAN_ZERO][BIT_ONE ], &filt_states[1], in, &f[1])
        )
        return false;

    RMS_OUT_DATA ra = 0, rb = 0;
    if (
            ! rms(window_size, &rms_states[0], f[0] - in, &ra)
        ||  ! rms(window_size, &rms_states[1], f[1] - in, &rb)
        )
        return false;

    if (ra < RMS_THRESHOLD || rb < RMS_THRESHOLD)
        return false;

    RUNS_OUT_DATA ro = 0;
    if (! runs(threshold, &run_state, ra, rb, &ro))
        return false;

    return decode(&dec_conf, &dec_state, offset, ro, out);
}

#ifndef __AVR__
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int top_main(int argc, char *argv[])
{
    if (argc != 4) {
        WARN("Supply summation window size, hysteresis, and sample offset");
        exit(EXIT_FAILURE);
    }

    char **arg = &argv[1];

    int window_size = strtol(*arg++, NULL, 0);
    int hysteresis = strtol(*arg++, NULL, 0);
    int offset = strtol(*arg++, NULL, 0);

    FILE *stream = stdin;
    while (true) {
        FILTER_IN_DATA in = 0;
        int result = fread(&in, sizeof in, 1, stream);

        if (result != 1) {
            if (feof(stream))
                break;

            WARN("fread failed");
            exit(EXIT_FAILURE);
        }

        DECODE_OUT_DATA out = EOF;
        if (top(window_size, hysteresis, offset, in, &out))
            putchar(out);
    }

    return 0;
}

#endif

