#include <stdlib.h>
#include <fftw3.h>
#include <math.h>

#include "audio.h"

#define BUFFER_SIZE 16384 * 16 * 32

#define WINDOW_SIZE 512

#define countof(X) (sizeof (X) / sizeof (X)[0])

int read_file(struct audio_state *a, const char *filename, size_t size, double input[size]);

double ref_data[BUFFER_SIZE],
       tst_data[BUFFER_SIZE];

static int do_fft(int sign, size_t in_size, fftw_complex input[in_size], size_t out_size, fftw_complex output[out_size])
{
    fftw_plan plan = fftw_plan_dft_1d(in_size, input, output, sign, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

    return 0;
}

static int complexify(size_t size, double input[size], fftw_complex output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i][0] = input[i];
        output[i][1] = 0.;
    }

    return 0;
}

static int realify(size_t size, fftw_complex input[size], double output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i] = sqrt(input[i][0] * input[i][1]);
    }

    return 0;
}

static int conjugate(size_t size, fftw_complex input[size], fftw_complex output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i][0] =  input[i][0];
        output[i][1] = -input[i][1];
    }

    return 0;
}

static int multiply(size_t size, fftw_complex A[size], fftw_complex B[size], fftw_complex C[size])
{
    for (size_t i = 0; i < size; i++) {
        // (a + bi) * (c + di)
        double a = A[i][0],
               b = A[i][1],
               c = B[i][0],
               d = B[i][1];
        C[i][0] = a * c - b * d;
        C[i][1] = b * c + a * d;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        abort();

    char *ref_file = argv[1],
         *tst_file = argv[2];
    
    size_t actual_size = 0;

    struct audio_state as = { .sample_offset = 0 };
    
    double (*buffer)[BUFFER_SIZE] = malloc(sizeof *buffer);

    fftw_complex *ref_data = fftw_malloc(countof(*buffer));
    actual_size = read_file(&as, ref_file, countof(*buffer), *buffer);
    complexify(countof(*buffer), *buffer, ref_data);
    
    fftw_complex *tst_data = fftw_malloc(countof(*buffer));
    read_file(&as, tst_file, countof(*buffer), *buffer);
    complexify(countof(*buffer), *buffer, tst_data);
    
    free(*buffer);

    fftw_complex (*ref_fft)[WINDOW_SIZE] = fftw_malloc(sizeof *ref_fft),
                 (*tst_fft)[WINDOW_SIZE] = fftw_malloc(sizeof *tst_fft),
                 (*cnj_fft)[WINDOW_SIZE] = fftw_malloc(sizeof *cnj_fft),
                 (*multed )[WINDOW_SIZE] = fftw_malloc(sizeof *multed );

    do_fft(FFTW_FORWARD, countof(*ref_data), ref_data, WINDOW_SIZE, *ref_fft);
    do_fft(FFTW_FORWARD, countof(*tst_data), tst_data, WINDOW_SIZE, *tst_fft);
    
    conjugate(WINDOW_SIZE, *ref_fft, *cnj_fft);

    multiply(WINDOW_SIZE, *multed, *cnj_fft, *tst_fft);
    fftw_free(*ref_fft);
    fftw_free(*tst_fft);
    fftw_free(*cnj_fft);

    fftw_complex (*reversed)[actual_size] = fftw_malloc(sizeof *reversed);
    double (*result)[WINDOW_SIZE] = malloc(sizeof *result);
    do_fft(FFTW_BACKWARD, countof(*multed), *multed, actual_size, *reversed);
    realify(actual_size, *reversed, *result);

    abort();

    return 0;
}

