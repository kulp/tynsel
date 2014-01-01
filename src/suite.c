#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "audio.h"
#include "decode.h"
#include "filters.h"
#include "streamdecode.h"

#define countof(X) (sizeof (X) / sizeof (X)[0])

extern int read_file(struct audio_state *a, const char *filename, size_t size, double input[size]);
extern int write_file_pcm(struct audio_state *a, const char *filename, size_t size, double output[size]);

static long find_start_bit(struct audio_state *as, size_t size, double in[size])
{
    size_t count = 0;

    for (unsigned i = 0; i < size; i++) {
        if (in[i] > 0)
            count++;
        else
            count = 0;

        if (count >= SAMPLES_PER_BIT(as) * 2) {
            for (unsigned j = i; j < size; j++) {
                if (in[j] == 0)
                    return j;
            }

            return -1;
        }
    }

    return -1;
}

static int process_read(struct audio_state *as, size_t size, double in[size])
{
    double *p = in;

    while (p < &in[size]) {
        long off = find_start_bit(as, size - (p - in), p);
        if (off < 0)
            break;

        p += off;
        int bits[11]; // start bit (0) + 7 data bits + 1 parity bit + 2 stop bits (11)
        for (unsigned i = 0; i < countof(bits); i++) {
            bits[i] = p[(int)(SAMPLES_PER_BIT(as) * (i + .5))] != 0;
        }
        if (bits[0] == 0 && bits[9] == 1 && bits[10] == 1) {
            int ascii = 0;
            int sum = 0;
            for (int i = 0; i < 7; i++) {
                int bit = bits[i + 1];
                ascii |= bit << i;
                sum ^= bit;
            }
            if (sum ^ bits[8]) // even parity failure
                printf("failed EVEN parity : ");
            printf("char '%c' (%d)\n", ascii, ascii);
        } else {
            puts("malformed word");
        }
        p += (int)(SAMPLES_PER_BIT(as) * 9); // leave stop bits for find_start_bit()
    }

    return 0;
}

static int emit(void *userdata, int status, int data)
{
    switch (status) {
        case STREAM_ERR_OK:
            printf("char '%c' (%d)\n", data, data);
            break;
        case STREAM_ERR_PARITY:
            printf("char '%c' (%d) (PARITY FAILED)\n", data, data);
            break;
        default:
            printf("unknown error\n");
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Supply input and output filenames\n");
        return EXIT_FAILURE;
    }

    const char *filename_in  = argv[1];
    const char *filename_out = argv[2];

    size_t bufsiz = 80000;
    double *buf = malloc(bufsiz * sizeof *buf);
    struct audio_state _as = {
        .baud_rate   = 300,
        .start_bits  = 1,
        .data_bits   = 7,
        .stop_bits   = 2,
        .parity_bits = 1,
    }, *as = &_as;
    read_file(as, filename_in, bufsiz, buf);

    double *dat = malloc(bufsiz * sizeof *dat);
#if 0
    struct filter_state *ch0    =   filter_create(FILTER_PASS_CHAN0),
                        *bit[2] = { filter_create(FILTER_PASS_CHAN0BIT0),
                                    filter_create(FILTER_PASS_CHAN0BIT1) };
    double *energies[2] = { malloc(bufsiz * sizeof **energies),
                            malloc(bufsiz * sizeof **energies) };
    const int window_size = 13; // TODO tune
    double cached_energy[2][window_size];

    double *edges = calloc(bufsiz, sizeof *edges);
    for (unsigned i = 0; i < bufsiz; i++) {
        filter_put(ch0, buf[i]);
        double bandpassed = filter_get(ch0);
        for (int b = 0; b < 2; b++) {
            filter_put(bit[b], bandpassed);
            double *energy = &energies[b][i];
            *energy = *(energy - 1);
            double *first_energy = &cached_energy[b][(i + window_size - 1) % window_size];
            if (i >= window_size)
                *energy -= *first_energy;

            double bitval = filter_get(bit[b]);
            double term = bitval * bitval;
            *first_energy = term;
            *energy += term;
        }

        if (i >= window_size)
            edges[i - window_size] = 0.5 * (energies[1][i] > energies[0][i]);
    }

    process_read(as, bufsiz - window_size, edges);

    filter_destroy(bit[1]);
    filter_destroy(bit[0]);
    filter_destroy(ch0);

    free(energies[1]);
    free(energies[0]);

    write_file_pcm(as, filename_out, bufsiz, edges);
#else
    struct stream_state *sd;
    streamdecode_init(&sd, as, NULL, emit);
    streamdecode_process(sd, bufsiz, buf);
    streamdecode_fini(sd);
#endif

    return 0;
}

