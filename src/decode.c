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

#include <sys/time.h>

#define ROUND_FACTOR(X,By)  (((X) + (By) - 1) / (By))
#define ROUND_HALF(X,By)    (((X) + (By / 2)) / (By))
#define ALL_BITS            (s->audio.start_bits + s->audio.data_bits + s->audio.parity_bits + s->audio.stop_bits)
#define BIT_PROB_THRESHOLD  0.1

static const size_t window_size = 512;

int decode_bits(struct decode_state *s, size_t size, double input[size], int output[ (size_t)(size / SAMPLES_PER_BIT(s) / ALL_BITS) ], double *offset, int channel, double *prob)
{
    fftw_complex *data       = fftw_malloc(window_size * sizeof *data);
    fftw_complex *fft_result = fftw_malloc(window_size * sizeof *fft_result);
    double *result_samples   = malloc(window_size * sizeof *result_samples);

    double p = 1.; // total probability for a string of bits
    int biti = 0;
    double end = size + *offset;
    // require more than a half-bit's worth of samples
    for (double dbb = *offset; end - dbb > SAMPLES_PER_BIT(s) / 2; dbb += SAMPLES_PER_BIT(s), biti++) {
        int word = biti / ALL_BITS;
        int wordbit = biti % ALL_BITS;
        if (wordbit == 0)
            output[word] = 0;

        fftw_plan plan_forward = fftw_plan_dft_1d(window_size, data, fft_result, FFTW_FORWARD, FFTW_ESTIMATE);

        size_t bit_base = dbb;
        *offset += dbb - bit_base;

        for (size_t i = 0; i < window_size; i++) {
            data[i][0] = i < SAMPLES_PER_BIT(s) ? input[bit_base + i] : 0.;
            data[i][1] = 0.;
        }

        fftw_execute(plan_forward);

        for (size_t i = 0; i < window_size; i++)
            result_samples[i] = sqrt(pow(fft_result[i][0],2) + pow(fft_result[i][1],2));

        // probable_bit is the sum of the probabilities for each bit,
        // disregarding channel ; positive for 1, negative for 0.
        double probable_bit = 0.;
        for (size_t i = 0; i < bit_recognisers_size; i++) {
            int bit;
            double prob = 0.;
            const struct bit_recogniser_rec *rec = &bit_recognisers[i];
            double *dat = (rec->flags & FFT_DATA) ? result_samples : &input[bit_base];
            struct timeval before, after, result;
            gettimeofday(&before, NULL);
            rec->rec(s, window_size, dat, &channel, &bit, &prob);
            gettimeofday(&after, NULL);
            timersub(&after, &before, &result);
            if (s->verbosity > 4)
                printf("bit recogniser %zd has prob %f for %d in %ld.%06ds\n", i, prob, bit, result.tv_sec, result.tv_usec);
            probable_bit += (bit * 2 - 1) * prob;
            p *= prob;
        }
        int bit = probable_bit > 0;
        if (s->verbosity > 4)
            printf("Bit probability product is %f for %d (raw %f)\n", fabs(probable_bit), bit, probable_bit);
        if (fabs(probable_bit) < BIT_PROB_THRESHOLD) {
            fprintf(stderr, "Bit probability %f below threshold %f\n", probable_bit, BIT_PROB_THRESHOLD);
            // TODO optionally return error code
        }
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

    if (prob)
        *prob = p;

    return 0;
}

// finds a 1-0 (high-freq to low-freq) edge naïvely : by finding a local
// maximum in the bit-recognising probability functions over a span of two
// bits' worth of samples
static int find_edge(struct decode_state *s, int channel, double *offset, size_t count, double input[count])
{
    double input_offset = *offset;
    double maxprob = 0.;
    int maxoff = -1;
    double prob = -1.;

    for (double bs_off = 0.; bs_off < count; bs_off += SAMPLES_PER_BIT(s)) {
        for (size_t samp_off = 0; samp_off < SAMPLES_PER_BIT(s) - input_offset; samp_off++) {
            int scratch = 0;
            double scratch_offset = input_offset;
            decode_bits(s, 2 * SAMPLES_PER_BIT(s), &input[(size_t)bs_off + samp_off], &scratch, &scratch_offset, channel, &prob);
            // find the point where probability is at its highest and the decoded
            // two-bit number is 0b01 (i.e. a 1-to-0 edge)
            if (scratch != 1)
                break;

            if (prob > maxprob) {
                maxprob = prob;
                // add one bit's worth because we look two ahead to find the
                // start bit and if we don't adjust it, we will be returning
                // the offset for the bit prior to the start bit
                maxoff = bs_off + samp_off + SAMPLES_PER_BIT(s);
            }
        }
    }

    *offset += maxoff;

    return 0;
}

int decode_data(struct decode_state *s, size_t count, double input[count])
{
    // TODO merge `offset` and `s->audio.sample_offset`
    double offset = 0.;
    int channel = -1;
    int rc = 0;

    rc = find_edge(s, channel, &offset, count, input);
    if (rc) {
        fprintf(stderr, "Failed to find any byte starts ; aborting\n");
        return -1;
    } else if (s->verbosity > 4)
        printf("edge offset is %.0f sample(s)\n", offset);

    size_t effective_count = count - offset;
    int output[ (size_t)(ROUND_HALF(effective_count / SAMPLES_PER_BIT(s), ALL_BITS)) ];

    decode_bits(s, effective_count - s->audio.sample_offset, input, output, &offset, channel, NULL);

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

