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
#include "common.h"

#include <stdlib.h>
#include <getopt.h>
#include <sndfile.h>
#include <math.h>

static int sample_rate = 44100;
// TODO make baud_rate configurable
static const int baud_rate = 300;
static unsigned start_bits  = 1,
                data_bits   = 8,
                parity_bits = 0,
                stop_bits   = 2;

#define PERBIN          ((double)sample_rate / SIZE)
#define SAMPLES_PER_BIT ((double)sample_rate / baud_rate)

static const double freqs[2][2] = {
    { 1070., 1270. },
    { 2025., 2225. },
};

static int verbosity;

struct put_state {
    int last_quadrant;
    double last_sample;
    double adjust;
};

static int put_sample(SNDFILE *sf, double freq, double gain, double sample_index, struct put_state *state)
{
    double proportion = sample_index / sample_rate;
    double radians = proportion * 2. * M_PI;
    double sample = sin(radians * freq) * gain;

    int rc = sf_write_double(sf, &sample, 1);
    state->last_sample = sample;

    return rc;
}

static int put_bit(SNDFILE *sf, double freq, double gain, struct put_state *state)
{
    double inverse = asin(state->last_sample / gain);
    switch (state->last_quadrant) {
        case 0: break;
        case 1:                                         // mirror around pi/2
        case 2: inverse = M_PI - inverse; break;        // mirror around 3*pi/2
        case 3: inverse = 2 * M_PI + inverse; break;    // mirror around 2pi
        default: abort(); // TODO just return 1 once we check put_bit()'s return value
    }
    double inverse_prop = inverse / (2 * M_PI);
    double samples_per_cycle = sample_rate / freq;
    // sample_offset gets +1 because we need to start at the *next* sample,
    // otherwise a sample will be duplicated
    double sample_offset = inverse_prop * samples_per_cycle + 1;

    double sidx;
    for (sidx = sample_offset; sidx < SAMPLES_PER_BIT + sample_offset + state->adjust; sidx++) {
        put_sample(sf, freq, gain, sidx, state);
    }

    state->adjust -= (sidx - sample_offset) - SAMPLES_PER_BIT;

    double si = sidx - 1; // undo last iteration of for-loop
    // TODO explain what is happening here
    state->last_quadrant = (si - floor(si / samples_per_cycle) * samples_per_cycle) / samples_per_cycle * 4;

    return 0;
}

int main(int argc, char* argv[])
{
    const char *output_file = NULL;
    unsigned channel = 1;
    double gain = 1.;

    int ch;
    while ((ch = getopt(argc, argv, "C:G:S:T:P:D:s:o:v")) != -1) {
        switch (ch) {
            case 'C': channel       = strtol(optarg, NULL, 0); break;
            case 'G': gain          = strtod(optarg, NULL);    break;
            case 'S': start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': data_bits     = strtol(optarg, NULL, 0); break;
            case 's': sample_rate   = strtol(optarg, NULL, 0); break;
            case 'o': output_file   = optarg;                  break;
            case 'v': verbosity++; break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    if (channel > countof(freqs)) {
        fprintf(stderr, "Invalid channel %d\n", channel);
        return -1;
    }

    if (!output_file) {
        fprintf(stderr, "No file specified to generate -- use `-o'\n");
        return -1;
    }

    SF_INFO sinfo = {
        .samplerate = sample_rate,
        .channels   = 1,
        .format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    };
    SNDFILE *sf = sf_open(output_file, SFM_WRITE, &sinfo);

    size_t index = 0;

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

        for (unsigned bit_index = 0; bit_index < start_bits; bit_index++) {
            put_bit(sf, freqs[channel][0], gain, &state);
        }

        for (unsigned bit_index = 0; bit_index < data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            double freq = freqs[channel][bit];
            put_bit(sf, freq, gain, &state);
        }

        // TODO parity bits

        for (unsigned bit_index = 0; bit_index < stop_bits; bit_index++) {
            put_bit(sf, freqs[channel][1], gain, &state);
        }
    }

    if (verbosity) {
        printf("read %zd items\n", index);
        printf("sample rate is %4d Hz\n", sample_rate);
        printf("baud rate is %4d\n", baud_rate);
        printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);
    }

    sf_close(sf);

    return 0;
}
