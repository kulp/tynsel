#include <stdlib.h>
#include <fftw3.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <getopt.h>

#include "audio.h"

#define BUFFER_SIZE 16384 * 3

#define WINDOW_SIZE 16000

#define countof(X) (sizeof (X) / sizeof (X)[0])

struct correlate_state {
    int channel;
    int verbosity;
    char *ref_file, *tst_file;
    struct {
        int has_edge;
        int compare;
    } action;
};

int read_file(struct audio_state *a, const char *filename, size_t size, double input[size]);

static int do_fft(int sign, size_t in_size, fftw_complex input[in_size], fftw_complex output[])
{
    // XXX should create plan before initialising data ; this is absolutely
    // necessary if we switch to FFTW_MEASURE
    fftw_plan plan = fftw_plan_dft_1d(in_size, input, output, sign, FFTW_ESTIMATE);
    fftw_execute(plan);
    fftw_destroy_plan(plan);

    return 0;
}

static void complexify(size_t size, double input[size], fftw_complex output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i][0] = input[i];
        output[i][1] = 0.;
    }
}

static void realify(size_t size, fftw_complex input[size], double output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i] = sqrt(pow(input[i][0], 2) + pow(input[i][1], 2));
    }
}

static void conjugate(size_t size, fftw_complex input[size], fftw_complex output[size])
{
    for (size_t i = 0; i < size; i++) {
        output[i][0] =  input[i][0];
        output[i][1] = -input[i][1];
    }
}

static void multiply(size_t size, fftw_complex C[size], fftw_complex A[size], fftw_complex B[size])
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
}

static size_t get_maxes(size_t size, double ins[size], size_t *count,
        double outs[*count], size_t indices[*count])
{
    outs[0] = -DBL_MAX;
    size_t im = 0;
    size_t c = 0;
    for (size_t i = 0; i < size; i++) {
        for (size_t check = 0; check < *count; check++) {
            if (ins[i] > outs[check]) {
                size_t outs_size = (*count - check - 1) * sizeof *outs;
                size_t inds_size = (*count - check - 1) * sizeof *indices;
                memmove(   &outs[check + 1],    &outs[check], outs_size);
                memmove(&indices[check + 1], &indices[check], inds_size);
                if (c < *count)
                    c++;
                outs[check] = ins[i];
                indices[check] = i;
                break;
            }
        }
    }

    *count = c;

    return im;
}

static int has_edge(struct audio_state *a, int chan, size_t size, fftw_complex data[size])
{
    return 0;
}

static int parse_opts(struct correlate_state *s, int argc, char *argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "C:r:t:" "ec" "v")) != -1) {
        switch (ch) {
            case 'C': s->channel = strtol(optarg, NULL, 0); break;
            case 'r': s->ref_file = optarg;                 break;
            case 't': s->tst_file = optarg;                 break;

            case 'e': s->action.has_edge = 1;               break;
            case 'c': s->action.compare  = 1;               break;

            case 'v': s->verbosity++;                       break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

int do_compare(size_t ref_size, fftw_complex *ref_data,
               size_t tst_size, fftw_complex *tst_fft,
               size_t *rst_size, fftw_complex **rst_data)
{
    fftw_complex (*ref_fft)[WINDOW_SIZE] = calloc(1, sizeof *ref_fft),
                 (*cnj_fft)[WINDOW_SIZE] = calloc(1, sizeof *cnj_fft),
                 (*multed )[WINDOW_SIZE] = calloc(1, sizeof *multed );

    do_fft(FFTW_FORWARD, WINDOW_SIZE, ref_data, *ref_fft);

    conjugate(WINDOW_SIZE, *ref_fft, *cnj_fft);
    free(*ref_fft);

    multiply(WINDOW_SIZE, *multed, *cnj_fft, tst_fft);
    free(*cnj_fft);

    fftw_complex (*reversed)[WINDOW_SIZE] = calloc(1, sizeof *reversed);
    do_fft(FFTW_BACKWARD, WINDOW_SIZE, *multed, *reversed);
    free(*multed);

    *rst_size = WINDOW_SIZE;
    *rst_data = *reversed;

    return 0;
}

int main(int argc, char *argv[])
{
    struct correlate_state _s = { .verbosity = 0 }, *s = &_s;
    struct audio_state as = { .sample_offset = 0 };

    parse_opts(s, argc, argv);

    if (!s->tst_file) {
        fprintf(stderr, "No test file specified with `-t`\n");
        exit(EXIT_FAILURE);
    }

    size_t ref_size = 0,
           tst_size = 0;

    double (*buffer)[BUFFER_SIZE] = calloc(1, sizeof *buffer);

    tst_size = read_file(&as, s->tst_file, countof(*buffer), *buffer);
    fftw_complex (*tst_data)[WINDOW_SIZE] = calloc(1, sizeof *tst_data);
    complexify(tst_size, *buffer, *tst_data);

    fftw_complex (*tst_fft)[WINDOW_SIZE] = calloc(1, sizeof *tst_fft);

    do_fft(FFTW_FORWARD, WINDOW_SIZE, *tst_data, *tst_fft);

    fftw_complex (*ref_data)[WINDOW_SIZE] = calloc(1, sizeof *ref_data);
    if (s->action.compare) {
        if (!s->ref_file) {
            fprintf(stderr, "No reference file specified with `-r`\n");
            exit(EXIT_FAILURE);
        }

        ref_size = read_file(&as, s->ref_file, countof(*buffer), *buffer);
        complexify(ref_size, *buffer, *ref_data);

        size_t result_size = 0;
        fftw_complex *cresult = NULL;
        do_compare(ref_size, *ref_data, WINDOW_SIZE, *tst_fft, &result_size, &cresult);
        free(*ref_data);

        double (*result)[WINDOW_SIZE] = malloc(sizeof *result);
        realify(WINDOW_SIZE, cresult, *result);
        free(cresult);

        size_t max_count = 10;
        double maxes[max_count];
        size_t imaxes[max_count];
        get_maxes(tst_size, *result, &max_count, maxes, imaxes);
        printf("0result[%5zd] = %e\n", 0, (*result)[0]);
        for (size_t i = 0; i < max_count; i++)
            printf(" result[%5zd] = %e\n", imaxes[i], (*result)[imaxes[i]]);

        free(*result);
    }

    free(*buffer);

    free(*tst_fft);
    free(*tst_data);

    return 0;
}

