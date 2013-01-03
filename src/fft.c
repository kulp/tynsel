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

#include "common.h"
#include "decode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>

#include <sndfile.h>

#define BUFFER_SIZE 16384 * 16 * 32

static int read_file(struct decode_state *s, const char *filename, size_t size, double input[size])
{
    size_t index = 0;
    SNDFILE *sf = NULL;
    SF_INFO sinfo = { .format = 0 };
    {
        sf = sf_open(filename, SFM_READ, &sinfo);
        if (!sf) {
            fprintf(stderr, "Failed to open `%s' : %s\n", filename, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // getting the sample rate from the file means right now the `-s' option on
        // the command line has no effect. In the future it might be removed, or it
        // might be necessary when the audio input has no accompanying sample-rate
        // information.
        s->audio.sample_rate = sinfo.samplerate;
    }

    {
        sf_count_t count = 0;
        do {
            double tmp[sinfo.channels];
            count = sf_read_double(sf, tmp, sinfo.channels);
            size_t where = (size_t)SAMPLES_PER_BIT(s) + index++;
            input[where] = tmp[0];
        } while (count && index < BUFFER_SIZE);

        if (sf_error(sf))
            sf_perror(sf);

        if (index >= BUFFER_SIZE)
            fprintf(stderr, "Warning, ran out of buffer space before reaching end of file\n");

        sf_close(sf);
    }

    return index;
}

static int parse_opts(struct decode_state *s, int argc, char *argv[], const char **filename)
{
    int ch;
    while ((ch = getopt(argc, argv, "S:T:P:D:s:O:v")) != -1) {
        switch (ch) {
            case 'S': s->audio.start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': s->audio.stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': s->audio.parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': s->audio.data_bits     = strtol(optarg, NULL, 0); break;
            case 's': s->audio.sample_rate   = strtol(optarg, NULL, 0); break;
            case 'O': s->audio.sample_offset = strtod(optarg, NULL);    break;
            case 'v': s->verbosity++; break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    if (fabs(s->audio.sample_offset) > SAMPLES_PER_BIT(s)) {
        fprintf(stderr, "sample offset (%f) > samples per bit (%4.1f)\n",
                s->audio.sample_offset, SAMPLES_PER_BIT(s));
        return -1;
    }

    if (optind < argc)
        *filename = argv[optind];

    return 0;
}

int main(int argc, char* argv[])
{
    struct decode_state _s = {
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
    }, *s = &_s;

    const char *filename = NULL;
    int rc = parse_opts(s, argc, argv, &filename);
    if (rc)
        return rc;

    if (!filename) {
        fprintf(stderr, "No files specified to process\n");
        return -1;
    }

    // The first SAMPLES_PER_BIT might be wrong because the sample_rate might
    // be wrong until the file is read. read_file() mends this and sets up the
    // correct offset.
    double *_input = calloc((size_t)SAMPLES_PER_BIT(s) * 2 + BUFFER_SIZE, sizeof *_input);
    size_t count = read_file(s, argv[optind], sizeof _input, _input);
    // TODO round up fractional samples ?
    double *input = &_input[(size_t)SAMPLES_PER_BIT(s) + (size_t)s->audio.sample_offset];

    if (s->verbosity) {
        printf("read %zd items\n", count);
        printf("sample rate is %4d Hz\n", s->audio.sample_rate);
        printf("baud rate is %4d\n", s->audio.baud_rate);
        printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT(s));
    }

    decode_data(s, count, input);

    free(_input);
    decode_cleanup();

    return 0;
}

