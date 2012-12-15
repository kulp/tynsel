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

#define _XOPEN_SOURCE 600
#include "encode.h"
#include "common.h"

#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#define SAMPLES_PER_BIT ((double)s->audio.sample_rate / s->audio.baud_rate)

struct put_state {
    int last_quadrant;
    double last_sample;
    double adjust;
};

static int encode_sample(struct encode_state *s, double freq, double sample_index, struct put_state *state)
{
    double proportion = sample_index / s->audio.sample_rate;
    double radians = proportion * 2. * M_PI;
    double sample = sin(radians * freq) * s->gain;

    int rc = sf_write_double(s->sf, &sample, 1);
    state->last_sample = sample;

    return rc;
}

static int encode_bit(struct encode_state *s, double freq, struct put_state *state)
{
    double inverse = asin(state->last_sample / s->gain);
    switch (state->last_quadrant) {
        case 0: break;
        case 1:                                         // mirror around pi/2
        case 2: inverse = M_PI - inverse; break;        // mirror around 3*pi/2
        case 3: inverse = 2 * M_PI + inverse; break;    // mirror around 2pi
        default: abort(); // TODO just return 1 once we check encode_bit()'s return value
    }
    double inverse_prop = inverse / (2 * M_PI);
    double samples_per_cycle = s->audio.sample_rate / freq;
    // sample_offset gets +1 because we need to start at the *next* sample,
    // otherwise a sample will be duplicated
    double sample_offset = inverse_prop * samples_per_cycle + 1;

    double sidx;
    for (sidx = sample_offset; sidx < SAMPLES_PER_BIT + sample_offset + state->adjust; sidx++) {
        encode_sample(s, freq, sidx, state);
    }

    state->adjust -= (sidx - sample_offset) - SAMPLES_PER_BIT;

    double si = sidx - 1; // undo last iteration of for-loop
    // TODO explain what is happening here
    state->last_quadrant = (si - floor(si / samples_per_cycle) * samples_per_cycle) / samples_per_cycle * 4;

    return 0;
}

static int parse_opts(struct encode_state *s, int argc, char *argv[], const char **filename)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:G:S:T:P:D:s:o:v")) != -1) {
        switch (ch) {
            case 'C': s->channel             = strtol(optarg, NULL, 0); break;
            case 'G': s->gain                = strtod(optarg, NULL);    break;
            case 'S': s->audio.start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': s->audio.stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': s->audio.parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': s->audio.data_bits     = strtol(optarg, NULL, 0); break;
            case 's': s->audio.sample_rate   = strtol(optarg, NULL, 0); break;
            case 'o': *filename              = optarg;                  break;
            case 'v': s->verbosity++;                                   break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    const char *output_file = NULL;
    struct encode_state _s = {
        .audio = {
            .sample_rate = 44100,
            .baud_rate   = 300, // TODO make baud_rate configurable
            .start_bits  = 1,
            .data_bits   = 8,
            .parity_bits = 0,
            .stop_bits   = 2,
            .freqs       = bell103_freqs,
        },
        .verbosity = 0,
        .channel   = 1,
        .gain      = 0.5,
    }, *s = &_s;

    int rc = parse_opts(s, argc, argv, &output_file);
    if (rc)
        return rc;
    if (!output_file) {
        fprintf(stderr, "No file specified to generate -- use `-o'\n");
        return -1;
    }

    if (s->channel > 1) {
        fprintf(stderr, "Invalid channel %d\n", s->channel);
        return -1;
    }

    {
        SF_INFO sinfo = {
            .samplerate = s->audio.sample_rate,
            .channels   = 1,
            .format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
        };
        s->sf = sf_open(output_file, SFM_WRITE, &sinfo);
        if (!s->sf) {
            fprintf(stderr, "Failed to open `%s' : %s\n", output_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    struct put_state state = { .last_quadrant = 0 };
    for (unsigned byte_index = 0; byte_index < (unsigned)argc - optind; byte_index++) {
        char *next = NULL;
        char *thing = argv[byte_index + optind];
        unsigned byte = strtol(thing, &next, 0);
        if (next == thing) {
            fprintf(stderr, "Error parsing argument at index %d, `%s'\n", optind, thing);
            return -1;
        }

        printf("writing byte %#x\n", byte);

        for (int bit_index = 0; bit_index < s->audio.start_bits; bit_index++) {
            encode_bit(s, s->audio.freqs[s->channel][0], &state);
        }

        for (int bit_index = 0; bit_index < s->audio.data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            double freq = s->audio.freqs[s->channel][bit];
            encode_bit(s, freq, &state);
        }

        // TODO parity bits

        for (int bit_index = 0; bit_index < s->audio.stop_bits; bit_index++) {
            encode_bit(s, s->audio.freqs[s->channel][1], &state);
        }
    }

    sf_close(s->sf);

    return 0;
}
