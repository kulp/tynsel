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
    int last;
    int edge;
    unsigned char bit;
    unsigned char byte;
    unsigned inited:1;
};

static void decode(struct state *s, int offset, int datum, int index)
{
    do {
        if (! s->inited) {
            s->inited = 1;
            break;
        }

        if (s->bit == 0 && datum >= THRESHOLD && s->last < THRESHOLD) {
            if (s->edge > 0 && index - s->edge < BITWIDTH) {
                WARN("found an edge at line %d, sooner than expected (last edge was %d)", index, s->edge);
            }
            s->edge = index;
        }

        if (s->edge < 0)
            break;

        if (s->edge + offset + s->bit * BITWIDTH == index) {
            // sample here
            int found = datum >= 0 ? 0 : 1;

            if (s->bit == 0) {
                // start bit
                if (found != 0) {
                    WARN("Start bit is not 0");
                    s->byte = 0;
                    s->bit = 0;
                    s->edge = 0;
                    break;
                }
            } else if (s->bit == 8) {
                // parity, skip
            } else if (s->bit > 8) {
                if (found != 1) {
                    WARN("Stop bit was not 1");
                    s->byte = 0;
                    s->bit = 0;
                    s->edge = 0;
                    break;
                }
            } else {
                s->byte |= found << (s->bit - 1);
            }

            if (s->bit >= 10) {
                putchar(s->byte);
                s->byte = 0;
                s->bit = 0;
                s->edge = 0;
                break;
            } else {
                s->bit++;
            }
        }
    } while (0);

    s->last = datum;
}

void decode_top(int offset, int datum)
{
    static struct state s = { .edge = -1 };
    static int index = 0;
    decode(&s, offset, datum, index++);
}

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

        decode_top(offset, i);
    }
}
