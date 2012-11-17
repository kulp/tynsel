#include <stdio.h>
#include <fftw3.h>
#include <math.h>
#include <float.h>

#include <sndfile.h>

#define BUFFER_SIZE 2048
#define SIZE 512

#define PERBIN (44100. / SIZE)
static const double lowend  = 1070 - PERBIN;
static const double highend = 2225 + PERBIN;
#define SAMPLE_RATE     44100
#define BAUD_RATE       300
#define SAMPLES_PER_BIT ((double)SAMPLE_RATE / BAUD_RATE)

#define countof(X) (sizeof (X) / sizeof (X)[0])

static const double freqs[] = {
    1070., 1270.,
    2025., 2225.,
};

int process_bit(size_t bit_base, fftw_complex *data, fftw_complex *fft_result, int *channel, int *bit)
{
    size_t maxi = -1;
    {
        double max = -1;
        for (int i = 0; i < SIZE; i++) {
            //printf("fft_result[%d] = { %2.2f, %2.2f }\n", i, fft_result[i][0], fft_result[i][1]);
            double mag = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));
            #if VERBOSE
            printf("magnitude[%d] = { %6.2f }\n", i, mag);
            #endif
            if (mag > max && (lowend / PERBIN) <= i && i <= (highend / PERBIN)) {
                max = mag;
                maxi = i;
            }
        }
    }

    #if VERBOSE
    printf("bucket with greatest magnitude was %zd, which corresponds to frequency range [%4.0f, %4.0f)\n",
            maxi, PERBIN * maxi, PERBIN * (maxi + 1));
    #endif

    double min = DBL_MAX;
    size_t mini = -1;
    double freq = maxi * PERBIN + (PERBIN / 2.);
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

    *channel = mini / 2 + 1;
    *bit = mini % 2;

    return 0;
}

int process_byte(size_t size, double input[size])
{
    fftw_complex *data       = fftw_malloc(SIZE * sizeof *data);
    fftw_complex *fft_result = fftw_malloc(SIZE * sizeof *fft_result);

    for (int bit = 0; bit < size / SAMPLES_PER_BIT; bit++) {
        fftw_plan plan_forward  = fftw_plan_dft_1d(SIZE, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        size_t bit_base = bit * SAMPLES_PER_BIT;

        for (int i = 0; i < SIZE; i++) {
            data[i][0] = i < SAMPLES_PER_BIT ? input[bit_base + i] : 0.;
            data[i][1] = 0.;
        }

        fftw_execute(plan_forward);

        int channel, bit;
        process_bit(bit_base, data, fft_result, &channel, &bit);
        printf("Guess : channel %zd bit %zd\n", channel, bit);

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
    printf("read %zd items\n", index);
    printf("sample rate is %4d Hz\n", SAMPLE_RATE);
    printf("baud rate is %4d\n", BAUD_RATE);
    printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);

    process_byte(index, input);

    return 0;
}

