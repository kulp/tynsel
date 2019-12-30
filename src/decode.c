#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define BITWIDTH 27 /* 8000 / 300 */
#define THRESHOLD 0

// TODO change FILTER_COEFF to fixed-point after proving algorithm
typedef float FILTER_COEFF;
#define FILTER_COEFF_read(Arg) parse_coeff_float(Arg)
typedef int16_t FILTER_IN_DATA;
typedef float FILTER_STATE_DATA;
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
#else
#include <stdio.h>
#define WARN(Fmt,...) fprintf(stderr, Fmt "\n", ##__VA_ARGS__)
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

bool decode_top(int8_t offset, DECODE_IN_DATA datum, DECODE_OUT_DATA *out)
{
    static struct bits_state s = { .off = -1, .last = THRESHOLD };
    static const struct bits_config c = {
        .start_bits  = 1,
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };
    return decode(&c, &s, offset, datum, out);
}

struct rms_state {
    RMS_OUT_DATA *window;
    RMS_OUT_DATA sum;
    uint8_t ptr;
    bool primed;
};

struct rms_config {
    uint8_t window_size;
};

// TODO rename -- we do not actually do the "root" part of RMS since it is
// expensive and for our purposes unnecessary.
static bool rms(const struct rms_config *c, struct rms_state *s, RMS_IN_DATA datum, RMS_OUT_DATA *out)
{
    s->sum -= s->window[s->ptr];
    s->window[s->ptr] = (RMS_OUT_DATA)(datum * datum);
    s->sum += s->window[s->ptr];

    if (s->ptr == c->window_size - 1)
        s->primed = true;

    // Avoid expensive modulo
    if (++s->ptr >= c->window_size)
        s->ptr = 0;

    if (s->primed)
        *out = s->sum;

    return s->primed;
}

struct runs_config {
    int8_t threshold;
};

struct runs_state {
    RUNS_OUT_DATA current;
};

static bool runs(const struct runs_config *c, struct runs_state *s, RUNS_IN_DATA da, RUNS_IN_DATA db, RUNS_OUT_DATA *out)
{
    int8_t inc = (da > db) ?  1 :
                 (da < db) ? -1 :
                              0 ;
    s->current = (RUNS_OUT_DATA)(s->current + inc);

    const int8_t max = (int8_t) c->threshold;
    const int8_t min = (int8_t)-c->threshold;

    if (s->current > max)
        s->current = max;

    if (s->current < min)
        s->current = min;

    *out = s->current;

    return true;
}

bool runs_top(int8_t threshold, RUNS_IN_DATA da, RUNS_IN_DATA db, RUNS_OUT_DATA *out)
{
    static struct runs_config c = { .threshold = 0 };
    static struct runs_state s = { .current = 0 };
    if (! c.threshold)
        c.threshold = threshold;

    return runs(&c, &s, da, db, out);
}

struct filter_config {
    FILTER_COEFF b[3], a[3];
};

struct filter_state {
    FILTER_IN_DATA in[3];
    FILTER_STATE_DATA out[3];
    uint8_t ptr;
};

static bool filter(const struct filter_config *c, struct filter_state *s, FILTER_IN_DATA datum, FILTER_OUT_DATA *out)
{
    s->in[s->ptr] = datum;

    // TODO obviate expensive modulo
    #define INDEX(x,n) (x)[(s->ptr + (n) + 3) % 3]

    s->out[s->ptr] = 0
        + c->b[0] * INDEX(s->in ,  0)
        + c->b[1] * INDEX(s->in , -1)
        + c->b[2] * INDEX(s->in , -2)

        // coefficient a0 is special, and does not appear here
        - c->a[1] * INDEX(s->out, -1)
        - c->a[2] * INDEX(s->out, -2)
        ;

    *out = (FILTER_OUT_DATA)s->out[s->ptr];

    ++s->ptr;
    if (s->ptr >= 3)
        s->ptr = 0;

    return true;
}

#ifndef __AVR__
#include <stdio.h>

static float parse_coeff_float(const char *arg)
{
    return strtof(arg, NULL);
}

int filter_main(int argc, char *argv[])
{
    if (argc != 7) {
        WARN("Supply six coefficients (b,b,b,a,a,a)");
        exit(EXIT_FAILURE);
    }

    struct filter_config c = { };

    char **arg = &argv[1];

    FILTER_COEFF *pb = c.b;
    *pb++ = FILTER_COEFF_read(*arg++);
    *pb++ = FILTER_COEFF_read(*arg++);
    *pb++ = FILTER_COEFF_read(*arg++);

    FILTER_COEFF *pa = c.a;
    *pa++ = FILTER_COEFF_read(*arg++);
    *pa++ = FILTER_COEFF_read(*arg++);
    *pa++ = FILTER_COEFF_read(*arg++);

    if (c.a[0] != 1) {
        WARN("A non-normalized filter is not supported");
        exit(EXIT_FAILURE);
    }

    struct filter_state s = { };

    while (!feof(stdin)) {
        int i = 0;
        int result = scanf("%d", &i);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        FILTER_OUT_DATA out = 0;
        if (filter(&c, &s, i, &out))
            printf("%d\n", out);
    }

    return 0;
}

int rms_main(int argc, char *argv[])
{
    if (argc != 2) {
        WARN("Supply a window size (in samples) as the first argument");
        exit(EXIT_FAILURE);
    }

    int n = strtol(argv[1], NULL, 0);

    RMS_OUT_DATA large[BITWIDTH] = { 0 }; // largest conceivable window size
    struct rms_config c = { .window_size = n };
    struct rms_state s = { .window = large };

    while (!feof(stdin)) {
        int i = 0;
        int result = scanf("%d", &i);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        RMS_OUT_DATA out = 0;
        if (rms(&c, &s, i, &out))
            printf("%d\n", out);
    }

    return 0;
}

int decode_main(int argc, char *argv[])
{
    if (argc != 2) {
        WARN("Supply a offset (in samples) from the start bit edge as the first argument");
        exit(EXIT_FAILURE);
    }

    int offset = strtol(argv[1], NULL, 0);

    while (!feof(stdin)) {
        int i = 0;
        int result = scanf("%d", &i);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        DECODE_OUT_DATA out = EOF;
        if (decode_top(offset, i, &out))
            putchar(out);
    }

    return 0;
}

int runs_main(int argc, char *argv[])
{
    if (argc != 4) {
        WARN("Supply a threshold count as the first argument, followed by two filenames");
        exit(EXIT_FAILURE);
    }

    int threshold = strtol(argv[1], NULL, 0);
    FILE *fa = fopen(argv[2], "r");
    FILE *fb = fopen(argv[3], "r");

    while (!feof(fa) && !feof(fb)) {
        RUNS_IN_DATA i = 0;
        int result;
        result = fscanf(fa, "%d", &i);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        RUNS_IN_DATA j = 0;
        result = fscanf(fb, "%d", &j);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        RUNS_OUT_DATA out = 0;
        if (runs_top(threshold, i, j, &out))
            printf("%d\n", out);
    }

    return 0;
}
#endif

