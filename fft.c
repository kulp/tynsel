#include "common.h"

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#include <fftw3.h>
#include <sndfile.h>

#define ROUND_FACTOR(X,By) (((X) + (By) - 1) / (By))

#define BUFFER_SIZE 16384 * 16

#define LOWEND          (s->freqs[0][0] - PERBIN)
#define HIGHEND         (s->freqs[1][1] + PERBIN)
#define PERBIN          ((double)s->sample_rate / s->fft_size)
#define SAMPLES_PER_BIT ((double)s->sample_rate / s->baud_rate)
#define ALL_BITS        (s->start_bits + s->data_bits + s->parity_bits + s->stop_bits)

struct detect_state {
    unsigned sample_rate;
    double sample_offset;
    const unsigned baud_rate;
    const unsigned fft_size;

    int start_bits,
        data_bits,
        parity_bits,
        stop_bits;

    int verbosity;

    const double (*freqs)[2];
};

static const double bell103_freqs[2][2] = {
    { 1070., 1270. },
    { 2025., 2225. },
};

static size_t get_max_magnitude(struct detect_state *s, fftw_complex *fft_result)
{
    size_t maxi = 0;
    double max = -1;
    for (size_t i = 0; i < s->fft_size; i++) {
        double mag = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));
        if (s->verbosity > 3) {
            printf("fft_result[%zd] = { %2.2f, %2.2f }\n", i, fft_result[i][0], fft_result[i][1]);
            printf("magnitude[%zd] = { %6.2f }\n", i, mag);
        }
        if (mag > max && (LOWEND / PERBIN) <= i && i <= (HIGHEND / PERBIN)) {
            max = mag;
            maxi = i;
        }
    }

    return maxi;
}

static void get_nearest_freq(struct detect_state *s, double freq, int *ch, int *f)
{
    double min = DBL_MAX;

    if (s->verbosity)
        printf("midpoint frequency is %4.0f\n", freq);

    for (unsigned chan = 0; chan < 2; chan++) {
        for (unsigned i = 0; i < 2; i++) {
            double t = fabs(freq - s->freqs[chan][i]);
            if (t < min) {
                min = t;
                *ch = chan;
                *f = i;
            }
        }
    }
}

int process_bit(struct detect_state *s, fftw_complex *fft_result, int *channel, int *bit)
{
    size_t maxi = get_max_magnitude(s, fft_result);

    if (s->verbosity) {
        printf("bucket with greatest magnitude was %zd, which corresponds to frequency range [%4.0f, %4.0f)\n",
                maxi, PERBIN * maxi, PERBIN * (maxi + 1));
    }

    double freq = maxi * PERBIN + (PERBIN / 2.);
    get_nearest_freq(s, freq, channel, bit);

    return 0;
}

int process_byte(struct detect_state *s, size_t size, double input[size], int output[ (size_t)(size / SAMPLES_PER_BIT / ALL_BITS) ], double *offset)
{
    fftw_complex *data       = fftw_malloc(s->fft_size * sizeof *data);
    fftw_complex *fft_result = fftw_malloc(s->fft_size * sizeof *fft_result);

    int biti = 0;
    for (double dbb = *offset; dbb < size + *offset; dbb += SAMPLES_PER_BIT, biti++) {
        int word = biti / ALL_BITS;
        int wordbit = biti % ALL_BITS;
        if (wordbit == 0)
            output[word] = 0;

        fftw_plan plan_forward = fftw_plan_dft_1d(s->fft_size, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        size_t bit_base = dbb;
        *offset += dbb - bit_base;

        for (size_t i = 0; i < s->fft_size; i++) {
            data[i][0] = i < SAMPLES_PER_BIT ? input[bit_base + i] : 0.;
            data[i][1] = 0.;
        }

        fftw_execute(plan_forward);

        int channel, bit;
        process_bit(s, fft_result, &channel, &bit);
        output[word] |= bit << wordbit;

        if (s->verbosity > 2) {
            printf("Guess : channel %d bit %d\n", channel, bit);
            int width = ROUND_FACTOR(ALL_BITS, 4);
            printf("output[%d] = 0x%0*x\n", word, width, output[word]);
        }

        fftw_destroy_plan(plan_forward);
    }

    fftw_free(data);
    fftw_free(fft_result);

    return 0;
}

int main(int argc, char* argv[])
{
    struct detect_state _s = {
        .sample_rate = 44100,
        .baud_rate   = 300, // TODO make baud_rate configurable
        .fft_size    = 512,
        .start_bits  = 1,
        .data_bits   = 8,
        .parity_bits = 0,
        .stop_bits   = 2,
        .verbosity   = 0,
        .freqs       = bell103_freqs,
    }, *s = &_s;

    {
        int ch;
        while ((ch = getopt(argc, argv, "S:T:P:D:s:O:v")) != -1) {
            switch (ch) {
                case 'S': s->start_bits    = strtol(optarg, NULL, 0); break;
                case 'T': s->stop_bits     = strtol(optarg, NULL, 0); break;
                case 'P': s->parity_bits   = strtol(optarg, NULL, 0); break;
                case 'D': s->data_bits     = strtol(optarg, NULL, 0); break;
                case 's': s->sample_rate   = strtol(optarg, NULL, 0); break;
                case 'O': s->sample_offset = strtod(optarg, NULL);    break;
                case 'v': s->verbosity++; break;
                default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
            }
        }

        if (fabs(s->sample_offset) > SAMPLES_PER_BIT) {
            fprintf(stderr, "sample offset (%f) > samples per bit (%4.1f)\n",
                    s->sample_offset, SAMPLES_PER_BIT);
            return -1;
        }

        if (optind >= argc) {
            fprintf(stderr, "No files specified to process\n");
            return -1;
        }
    }

    SNDFILE *sf = NULL;
    {
        SF_INFO sinfo = { .format = 0 };
        sf = sf_open(argv[optind], SFM_READ, &sinfo);
        if (!sf) {
            fprintf(stderr, "Failed to open `%s' : %s\n", argv[optind], strerror(errno));
            exit(EXIT_FAILURE);
        }

        // getting the sample rate from the file means right now the `-s' option on
        // the command line has no effect. In the future it might be removed, or it
        // might be necessary when the audio input has no accompanying sample-rate
        // information.
        s->sample_rate = sinfo.samplerate;
    }

    double *_input = calloc((size_t)SAMPLES_PER_BIT * 2 + BUFFER_SIZE, sizeof *_input);
    double *input = &_input[(size_t)SAMPLES_PER_BIT + (size_t)s->sample_offset];

    size_t index = 0;
    {
        sf_count_t count = 0;
        do {
            count = sf_read_double(sf, &_input[(size_t)SAMPLES_PER_BIT + index++], 1);
        } while (count && index < BUFFER_SIZE);

        if (index >= BUFFER_SIZE)
            fprintf(stderr, "Warning, ran out of buffer space before reaching end of file\n");

        sf_close(sf);
    }

    if (s->verbosity) {
        printf("read %zd items\n", index);
        printf("sample rate is %4d Hz\n", s->sample_rate);
        printf("baud rate is %4d\n", s->baud_rate);
        printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);
    }

    {
        int output[ (size_t)(index / SAMPLES_PER_BIT / ALL_BITS) ];

        // TODO merge `offset` and `s->sample_offset`
        double offset = 0.;
        process_byte(s, index - s->sample_offset, input, output, &offset);
        for (size_t i = 0; i < countof(output); i++) {
            if (output[i] & ((1 << s->start_bits) - 1))
                fprintf(stderr, "Start bit%s %s not zero\n",
                        s->start_bits > 1 ? "s" : "", s->start_bits > 1 ? "were" : "was");
            if (output[i] >> (ALL_BITS - s->stop_bits) != (1 << s->stop_bits) - 1)
                fprintf(stderr, "Stop bits were not one\n");
            int width = ROUND_FACTOR(s->data_bits, 4);
            printf("output[%zd] = 0x%0*x\n", i, width, (output[i] >> s->start_bits) & ((1u << s->data_bits) - 1));
        }
    }

    free(_input);
    fftw_cleanup();

    return 0;
}

