#include <stdio.h>
#include <fftw3.h>
#include <math.h>
#include <float.h>

#include <sndfile.h>

#define BUFFER_SIZE 2048
#define SIZE 512

#define PERBIN          (44100. / SIZE)
#define LOWEND          (freqs[0] - PERBIN)
#define HIGHEND         (freqs[3] + PERBIN)
#define SAMPLE_RATE     44100
#define BAUD_RATE       300
#define SAMPLES_PER_BIT ((double)SAMPLE_RATE / BAUD_RATE)
#define START_BITS      1
#define DATA_BITS       8
#define PARITY_BITS     0
#define STOP_BITS       2
#define ALL_BITS        (START_BITS + DATA_BITS + PARITY_BITS + STOP_BITS)

#define countof(X) (sizeof (X) / sizeof (X)[0])

static const double freqs[] = {
    1070., 1270.,
    2025., 2225.,
};

static size_t get_max_magnitude(fftw_complex *fft_result)
{
    size_t maxi = 0;
    double max = -1;
    for (size_t i = 0; i < SIZE; i++) {
        double mag = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));
        #if VERBOSE > 3
        printf("fft_result[%zd] = { %2.2f, %2.2f }\n", i, fft_result[i][0], fft_result[i][1]);
        printf("magnitude[%zd] = { %6.2f }\n", i, mag);
        #endif
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
    #if VERBOSE
    printf("midpoint frequency is %4.0f\n", freq);
    #endif
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

    #if VERBOSE
    printf("bucket with greatest magnitude was %zd, which corresponds to frequency range [%4.0f, %4.0f)\n",
            maxi, PERBIN * maxi, PERBIN * (maxi + 1));
    #endif

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

        #if VERBOSE > 2
        printf("Guess : channel %zd bit %zd\n", channel, bit);
        printf("output[%zd] = %#x\n", word, output[word]);
        #endif

        fftw_destroy_plan(plan_forward);
    }

    fftw_free(data);
    fftw_free(fft_result);

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
        return -1;

    SF_INFO sinfo = { 0 };
    SNDFILE *sf = sf_open(argv[1], SFM_READ, &sinfo);

    double input[BUFFER_SIZE] = { 0 };

    sf_count_t count = 0;
    size_t index = 0;
    do {
        count = sf_read_double(sf, &input[index++], 1);
    } while (count && index < BUFFER_SIZE);
    #if VERBOSE
    printf("read %zd items\n", index);
    printf("sample rate is %4d Hz\n", SAMPLE_RATE);
    printf("baud rate is %4d\n", BAUD_RATE);
    printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);
    #endif

    int output[(index + (size_t)SAMPLES_PER_BIT - 1) / (size_t)SAMPLES_PER_BIT / ALL_BITS];

    process_byte(index, input, output);
    for (int i = 0; i < countof(output); i++) {
        if (output[i] & 1)
            fprintf(stderr, "Start bit was not zero\n");
        if (output[i] >> (START_BITS + DATA_BITS) != (1 << STOP_BITS) - 1)
            fprintf(stderr, "Stop bits were not one\n");
        printf("output[%zd] = 0x%02x\n", i, (output[i] >> 1) & 0xff);
    }

    sf_close(sf);

    fftw_cleanup();

    return 0;
}

