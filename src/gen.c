/*
 * Copyright (c) 2012-2020 Darren Kulp
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

#include "encode.h"

#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define int16_tMAX INT16_MAX
#define  int8_tMAX  INT8_MAX

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

typedef ENCODE_DATA_TYPE DATA_TYPE;

// Creates a quarter-wave sine table, scaled by the given gain.
// Valid indices into the table are [0,WAVE_TABLE_SIZE).
// Input indices are augmented by 0.5 before computing the sine, on the
// assumption that WAVE_TABLE_SIZE is a power of two and that we want to do
// arithmetic with powers of two. If we did not compensate somehow, we would
// either have two entries for zero (when flipping the quadrant), two entries
// for max (also during flipping), or an uneven gap between the minimum
// positive and minimum negative output values.
static void make_sine_table(DATA_TYPE sines[WAVE_TABLE_SIZE], float gain)
{
    const DATA_TYPE max = (CAT(ENCODE_DATA_TYPE,MAX) / 2);
    for (unsigned int i = 0; i < WAVE_TABLE_SIZE; i++) {
        sines[i] = (DATA_TYPE)(gain * sinf(2 * M_PI * (i + 0.5) / MAJOR_PER_CYCLE) * max - 1);
    }
}

static int parse_opts(struct encode_state *s, int argc, char *argv[], const char **filename)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:G:S:T:P:D:p:s:o:" "v")) != -1) {
        switch (ch) {
            case 'C': s->channel             = strtol(optarg, NULL, 0); break;
            case 'G': s->gain                = strtof(optarg, NULL);    break;
            case 'S': s->audio.start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': s->audio.stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': s->audio.parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': s->audio.data_bits     = strtol(optarg, NULL, 0); break;
            case 'p': s->audio.parity        = strtol(optarg, NULL, 0); break;
            case 'o': *filename              = optarg;                  break;

            case 'v': s->verbosity++;                                   break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

// returns zero on failure
static unsigned int parse_number(const char *in, char **next, int base)
{
    if (!strncmp(in, "0b", 2)) {
        char *nn = NULL;
        unsigned result = strtol(in + 2, &nn, 2);
        if (nn == in + 2)
            return 0;
        *next = nn;
        return result;
    }
    return strtol(in, next, base);
}

int main(int argc, char* argv[])
{
    const char *output_file = NULL;
    struct encode_state _s = {
        .audio = {
            .start_bits  = 1,
            .data_bits   = 8,
            .parity_bits = 0,
            .stop_bits   = 2,
        },
        .verbosity = 0,
        .channel   = 0,
        .gain      = 0.5,
    }, *s = &_s;

    int rc = parse_opts(s, argc, argv, &output_file);
    if (rc)
        return rc;

    if (s->channel > 1) {
        fprintf(stderr, "Invalid channel %d\n", s->channel);
        return -1;
    }

    if (!output_file && s->verbosity)
        fprintf(stderr, "No file specified to generate -- using stdout\n");

    FILE *stream = output_file ? fopen(output_file, "w") : stdout;
    if (! stream) {
        fprintf(stderr, "Failed to open `%s' : %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int byte_count = argc - optind;
    unsigned bytes[byte_count];
    for (int byte_index = 0; byte_index < byte_count; byte_index++) {
        char *next = NULL;
        char *thing = argv[byte_index + optind];
        bytes[byte_index] = parse_number(thing, &next, 0);
        if (next == thing) {
            fprintf(stderr, "Error parsing argument at index %d, `%s'\n", optind, thing);
            return -1;
        }
    }

    DATA_TYPE sines[WAVE_TABLE_SIZE];
    make_sine_table(sines, s->gain);
    s->byte_state.bit_state.sample_state.quadrant = &sines;

    for (size_t i = 0; i < SAMPLES_PER_BIT; /* incremented inside loop */) {
        DATA_TYPE out = 0;
        if (encode_carrier(s, true, s->channel, &out))
            i++;
        fwrite(&out, sizeof out, 1, stream);
    }

    for (int b = 0; b < byte_count; /* incremented inside loop */) {
        DATA_TYPE out = 0;
        if (encode_bytes(s, true, s->channel, bytes[b], &out))
            b++;
        fwrite(&out, sizeof out, 1, stream);
    }

    for (size_t i = 0; i < SAMPLES_PER_BIT; /* incremented inside loop */) {
        DATA_TYPE out = 0;
        if (encode_carrier(s, true, s->channel, &out))
            i++;
        fwrite(&out, sizeof out, 1, stream);
    }

    // drain the encoder
    {
        DATA_TYPE out = 0;
        while (! encode_carrier(s, true, s->channel, &out))
            fwrite(&out, sizeof out, 1, stream);
    }

    return 0;
}
