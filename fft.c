#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <fftw3.h>
#include <sndfile.h>

#define ROUND_FACTOR(X,By) (((X) + (By) - 1) / (By))

#define BUFFER_SIZE 16384
#define SIZE 512

static int sample_rate = 44100;
#define PERBIN          ((double)sample_rate / SIZE)
#define LOWEND          (freqs[2] - PERBIN)
#define HIGHEND         (freqs[3] + PERBIN)
#define BAUD_RATE       300
#define SAMPLES_PER_BIT ((double)sample_rate / BAUD_RATE)
static int start_bits  = 1,
           data_bits   = 8,
           parity_bits = 0,
           stop_bits   = 2;
#define ALL_BITS        (start_bits + data_bits + parity_bits + stop_bits)

#define countof(X) (sizeof (X) / sizeof (X)[0])

static const double freqs[] = {
    1070., 1270.,
    2025., 2225.,
};

static int verbosity;

static size_t get_max_magnitude(fftw_complex *fft_result)
{
    size_t maxi = 0;
    double max = -1;
    for (size_t i = 0; i < SIZE; i++) {
        double mag = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));
        if (verbosity > 3) {
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

static size_t get_nearest_freq(double freq)
{
    size_t mini = 0;
    double min = DBL_MAX;

    if (verbosity)
        printf("midpoint frequency is %4.0f\n", freq);

    for (int i = 0; i < countof(freqs); i++) {
        double t = fabs(freq - freqs[i]);
        if (t < min) {
            min = t;
            mini = i;
        }
    }

    return mini;
}

int process_bit(size_t bit_base, fftw_complex *fft_result, int *channel, int *bit)
{
    size_t maxi = get_max_magnitude(fft_result);

    if (verbosity) {
        printf("bucket with greatest magnitude was %zd, which corresponds to frequency range [%4.0f, %4.0f)\n",
                maxi, PERBIN * maxi, PERBIN * (maxi + 1));
    }

    double freq = maxi * PERBIN + (PERBIN / 2.);
    size_t mini = get_nearest_freq(freq);

    *channel = mini / 2 + 1;
    *bit = mini % 2;

    return 0;
}

int process_byte(size_t size, double input[size], int output[size / (size_t)SAMPLES_PER_BIT / ALL_BITS])
{
    fftw_complex *data       = fftw_malloc(SIZE * sizeof *data);
    fftw_complex *fft_result = fftw_malloc(SIZE * sizeof *fft_result);

    for (int biti = 0; biti < size / SAMPLES_PER_BIT; biti++) {
        int word = biti / ALL_BITS;
        int wordbit = biti % ALL_BITS;
        if (wordbit == 0)
            output[word] = 0;

        fftw_plan plan_forward = fftw_plan_dft_1d(SIZE, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        size_t bit_base = biti * SAMPLES_PER_BIT;

        for (int i = 0; i < SIZE; i++) {
            data[i][0] = i < SAMPLES_PER_BIT ? input[bit_base + i] : 0.;
            data[i][1] = 0.;
        }

        fftw_execute(plan_forward);

        int channel, bit;
        process_bit(bit_base, fft_result, &channel, &bit);
        output[word] |= bit << wordbit;

        if (verbosity > 2) {
            printf("Guess : channel %zd bit %zd\n", channel, bit);
            int width = ROUND_FACTOR(ALL_BITS, 4);
            printf("output[%zd] = 0x%0*x\n", word, width, output[word]);
        }

        fftw_destroy_plan(plan_forward);
    }

    fftw_free(data);
    fftw_free(fft_result);

    return 0;
}

int main(int argc, char* argv[])
{
    int sample_offset = 0;

    int ch;
    while ((ch = getopt(argc, argv, "S:T:P:D:s:O:v")) != -1) {
        switch (ch) {
            case 'S': start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': data_bits     = strtol(optarg, NULL, 0); break;
            case 's': sample_rate   = strtol(optarg, NULL, 0); break;
            case 'O': sample_offset = strtol(optarg, NULL, 0); break;
            case 'v': verbosity++; break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    if (abs(sample_offset) > SAMPLES_PER_BIT) {
        fprintf(stderr, "sample offset (%d) > samples per bit (%4.1f)\n",
                sample_offset, SAMPLES_PER_BIT);
        return -1;
    }

    SF_INFO sinfo = { 0 };
    SNDFILE *sf = sf_open(argv[optind], SFM_READ, &sinfo);

    double _input[(size_t)SAMPLES_PER_BIT * 2 + BUFFER_SIZE];
    memset(_input, 0, sizeof _input);
    double *input = &_input[(size_t)SAMPLES_PER_BIT + sample_offset];

    sf_count_t count = 0;
    size_t index = 0;
    do {
        count = sf_read_double(sf, &_input[(size_t)SAMPLES_PER_BIT + index++], 1);
    } while (count && index < BUFFER_SIZE);

    if (verbosity) {
        printf("read %zd items\n", index);
        printf("sample rate is %4d Hz\n", sample_rate);
        printf("baud rate is %4d\n", BAUD_RATE);
        printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);
    }

    int output[ ROUND_FACTOR( ROUND_FACTOR(index, (size_t)SAMPLES_PER_BIT), ALL_BITS ) ];

    process_byte(index - sample_offset, input, output);
    for (int i = 0; i < countof(output); i++) {
        if (output[i] & ((1 << start_bits) - 1))
            fprintf(stderr, "Start bit%s %s not zero\n",
                    start_bits > 1 ? "s" : "", start_bits > 1 ? "were" : "was");
        if (output[i] >> (start_bits + data_bits) != (1 << stop_bits) - 1)
            fprintf(stderr, "Stop bits were not one\n");
        int width = ROUND_FACTOR(data_bits, 4);
        printf("output[%zd] = 0x%0*x\n", i, width, (output[i] >> start_bits) & ((1u << data_bits) - 1));
    }

    sf_close(sf);

    fftw_cleanup();

    return 0;
}

