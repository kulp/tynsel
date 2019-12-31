/*
 * Copyright (c) 2012-2014 Darren Kulp
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
#include <string.h>
#include <errno.h>

#include <sndfile.h>

static int parse_opts(struct encode_state *s, int argc, char *argv[], const char **filename)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:G:S:T:P:D:s:I:L:o:" "v")) != -1) {
        switch (ch) {
            case 'C': s->channel             = strtol(optarg, NULL, 0); break;
            case 'G': s->gain                = strtod(optarg, NULL);    break;
            case 'S': s->audio.start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': s->audio.stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': s->audio.parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': s->audio.data_bits     = strtol(optarg, NULL, 0); break;
            case 's': s->audio.sample_rate   = strtol(optarg, NULL, 0); break;
            case 'I': s->index               = strtol(optarg, NULL, 0); break;
            case 'L': s->length              = strtol(optarg, NULL, 0); break;
            case 'o': *filename              = optarg;                  break;

            case 'v': s->verbosity++;                                   break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

static int sample_callback(struct audio_state *a, size_t count, double samples[count], void *userdata)
{
    (void)a;
    return sf_write_double(userdata, samples, count);
}

static int parse_number(const char *in, char **next, int base)
{
    if (!strncmp(in, "0b", 2)) {
        char *nn = NULL;
        int result = strtol(in + 2, &nn, 2);
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
        .cb.put_samples = sample_callback,
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

    SNDFILE *sf;
    {
        SF_INFO sinfo = {
            .samplerate = s->audio.sample_rate,
            .channels   = 1,
            .format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
        };
        sf = s->cb.userdata = sf_open(output_file, SFM_WRITE, &sinfo);
        if (!sf) {
            fprintf(stderr, "Failed to open `%s' : %s\n", output_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    size_t byte_count = argc - optind;
    unsigned bytes[byte_count];
    for (unsigned byte_index = 0; byte_index < byte_count; byte_index++) {
        char *next = NULL;
        char *thing = argv[byte_index + optind];
        bytes[byte_index] = parse_number(thing, &next, 0);
        if (next == thing) {
            fprintf(stderr, "Error parsing argument at index %d, `%s'\n", optind, thing);
            return -1;
        }
    }

    double zero = 0.;
    for (int i = 0; i < s->index; i++)
        // TODO put more samples at once
        s->cb.put_samples(&s->audio, 1, &zero, s->cb.userdata);

    encode_carrier(s, 20);

    int samples = encode_bytes(s, byte_count, bytes);
    if (samples < 0)
        fprintf(stderr, "Error while encoding %zd bytes : %s\n", byte_count, strerror(errno));

    encode_carrier(s, 20);

    for (int i = samples + s->index; i < s->length; i++)
        s->cb.put_samples(&s->audio, 1, &zero, s->cb.userdata);

    sf_close(sf);

    return 0;
}
