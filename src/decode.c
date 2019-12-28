#include <stdbool.h>
#include <stdlib.h>

#define BITWIDTH 27 /* 8000 / 300 */
#define THRESHOLD 0

#ifdef __AVR__
#define WARN(...) (void)(__VA_ARGS__)
#else
#include <stdio.h>
#define WARN(Fmt,...) fprintf(stderr, Fmt "\n", ##__VA_ARGS__)
#endif

struct bits_state {
    signed char off;
    signed char last;
    unsigned char bit;
    unsigned char byte;
};

struct bits_config {
    unsigned char start_bits;
    unsigned char data_bits;
    unsigned char parity_bits;
    unsigned char stop_bits;
};

static bool decode(const struct bits_config *c, struct bits_state *s, int offset, int datum, char *out)
{
    const unsigned char before_parity = c->start_bits + c->data_bits;
    const unsigned char before_stop   = before_parity + c->parity_bits;
    do {
        if (s->bit == 0 && datum >= THRESHOLD && s->last < THRESHOLD) {
            s->off = offset;
        }

        if (s->off < 0)
            break;

        if (s->off == 0) {
            // sample here
            int found = datum >= 0 ? 0 : 1;

            if (s->bit < c->start_bits) {
                // start bit(s)
                if (found != 0) {
                    WARN("Start bit is not 0");
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else if (s->bit >= before_parity && s->bit < before_stop) {
                // parity, skip
            } else if (s->bit >= before_stop) {
                if (found != 1) {
                    WARN("Stop bit was not 1");
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else {
                s->byte |= found << (s->bit - 1);
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

bool decode_top(int offset, int datum, char *out)
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
    int *window;
    int sum;
    unsigned char ptr;
    bool primed;
};

struct rms_config {
    unsigned char window_size;
};

static bool rms(const struct rms_config *c, struct rms_state *s, int datum, int *out)
{
    s->sum -= s->window[s->ptr];
    s->window[s->ptr] = datum * datum;
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

// TODO rename -- we do not actually do the "root" part of RMS since it is
// expensive and for our purposes unnecessary.
bool rms_top(unsigned char window_size, int datum, int *out)
{
    static int large[BITWIDTH]; // largest conceivable window size
    static struct rms_config c;
    static struct rms_state s = { .window = large };
    if (! c.window_size)
        c.window_size = window_size;

    return rms(&c, &s, datum, out);
}

#ifndef __AVR__
#include <stdio.h>

int rms_main(int argc, char *argv[])
{
    if (argc != 2) {
        WARN("Supply a window size (in samples) as the first argument");
        exit(EXIT_FAILURE);
    }

    int n = strtol(argv[1], NULL, 0);

    while (!feof(stdin)) {
        int i = 0;
        int result = scanf("%d", &i);
        if (result == EOF)
            break;

        if (result != 1) {
            WARN("scanf failed");
            exit(EXIT_FAILURE);
        }

        int out = 0;
        if (rms_top(n, i, &out))
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

        char out = EOF;
        if (decode_top(offset, i, &out))
            putchar(out);
    }

    return 0;
}
#endif

