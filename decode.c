/*
 * Copyright (c) 2012 Darren Kulp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "decode.h"
#include "decoders.h"
#include "common.h"

#include <math.h>
#include <fftw3.h>
#include <float.h>
#include <stdlib.h>

#define ROUND_FACTOR(X,By) (((X) + (By) - 1) / (By))
#define ALL_BITS        (s->audio.start_bits + s->audio.data_bits + s->audio.parity_bits + s->audio.stop_bits)

static const size_t fft_size = 512;

int decode_byte(struct decode_state *s, size_t size, double input[size], int output[ (size_t)(size / SAMPLES_PER_BIT(s) / ALL_BITS) ], double *offset, int channel)
{
    fftw_complex *data       = fftw_malloc(fft_size * sizeof *data);
    fftw_complex *fft_result = fftw_malloc(fft_size * sizeof *fft_result);
    double *result_samples   = malloc(fft_size * sizeof *result_samples);

    int biti = 0;
    for (double dbb = *offset; dbb < size + *offset; dbb += SAMPLES_PER_BIT(s), biti++) {
        int word = biti / ALL_BITS;
        int wordbit = biti % ALL_BITS;
        if (wordbit == 0)
            output[word] = 0;

        fftw_plan plan_forward = fftw_plan_dft_1d(fft_size, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        size_t bit_base = dbb;
        *offset += dbb - bit_base;

        for (size_t i = 0; i < fft_size; i++) {
            data[i][0] = i < SAMPLES_PER_BIT(s) ? input[bit_base + i] : 0.;
            data[i][1] = 0.;
        }

        fftw_execute(plan_forward);

        for (size_t i = 0; i < fft_size; i++)
            result_samples[i] = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));

        // probable_bit is the sum of the probabilities for each bit,
        // disregarding channel ; positive for 1, negative for 0.
        double probable_bit = 0.;
        for (size_t i = 0; i < bit_recognisers_size; i++) {
            int bit;
            double prob = 0.;
            bit_recogniser *rec = bit_recognisers[i];
            rec(s, fft_size, result_samples, &channel, &bit, &prob);
            probable_bit += (bit * 2 - 1) * prob;
        }
        int bit = probable_bit > 0;
        output[word] |= bit << wordbit;

        if (s->verbosity > 2) {
            printf("Guess : channel %d bit %d\n", channel, bit);
            int width = ROUND_FACTOR(ALL_BITS, 4);
            printf("output[%d] = 0x%0*x\n", word, width, output[word]);
        }

        fftw_destroy_plan(plan_forward);
    }

    free(result_samples);
    fftw_free(fft_result);
    fftw_free(data);

    return 0;
}

int decode_data(struct decode_state *s, size_t count, double input[count])
{
    int output[ (size_t)(count / SAMPLES_PER_BIT(s) / ALL_BITS) ];

    // TODO merge `offset` and `s->audio.sample_offset`
    double offset = 0.;
    decode_byte(s, count - s->audio.sample_offset, input, output, &offset, -1);
    for (size_t i = 0; i < countof(output); i++) {
        if (output[i] & ((1 << s->audio.start_bits) - 1))
            fprintf(stderr, "Start bit%s %s not zero\n",
                    s->audio.start_bits > 1 ? "s" : "", s->audio.start_bits > 1 ? "were" : "was");
        if (output[i] >> (ALL_BITS - s->audio.stop_bits) != (1 << s->audio.stop_bits) - 1)
            fprintf(stderr, "Stop bits were not one\n");
        int width = ROUND_FACTOR(s->audio.data_bits, 4);
        printf("output[%zd] = 0x%0*x\n", i, width, (output[i] >> s->audio.start_bits) & ((1u << s->audio.data_bits) - 1));
    }

    return 0;
}

void decode_cleanup(void)
{
    fftw_cleanup();
}

