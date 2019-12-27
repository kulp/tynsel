#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define BITWIDTH 27 /* 8000 / 300 */
#define THRESHOLD 0

#ifdef __AVR__
#define WARN(...) (void)(__VA_ARGS__)
#else
#define WARN(Fmt,...) fprintf(stderr, Fmt "\n", ##__VA_ARGS__)
#endif

struct state {
    int off;
    signed char last;
    unsigned char bit;
    unsigned char byte;
};

static bool decode(struct state *s, int offset, int datum, char *out)
{
    do {
        if (s->bit == 0 && datum >= THRESHOLD && s->last < THRESHOLD) {
            s->off = offset;
        }

        if (s->off < 0)
            break;

        if (s->off == 0) {
            // sample here
            int found = datum >= 0 ? 0 : 1;

            if (s->bit == 0) {
                // start bit
                if (found != 0) {
                    WARN("Start bit is not 0");
                    s->byte = 0;
                    s->bit = 0;
                    s->off = -1;
                    break;
                }
            } else if (s->bit == 8) {
                // parity, skip
            } else if (s->bit > 8) {
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
    static struct state s = { .off = -1, .last = THRESHOLD };
    return decode(&s, offset, datum, out);
}

#ifndef __AVR__
int main(int argc, char *argv[])
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
}
#endif

