/*
 * Copyright (c) 2019-2020 Darren Kulp
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

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static FILE *open_file(const char *filename, const char *mode, FILE *dflt)
{
    if (strcmp(filename, "-") == 0)
        return dflt;

    return fopen(filename, mode);
}

static int parse_opts(AUDIO_CONFIG *c, int argc, char *argv[], FILE **input_stream, FILE **output_stream)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:W:T:H:O:F:o:")) != -1) {
        switch (ch) {
            case 'C': c->channel     = strtol(optarg, NULL, 0);         break;
            case 'W': c->window_size = strtol(optarg, NULL, 0);         break;
            case 'T': c->threshold   = strtol(optarg, NULL, 0);         break;
            case 'H': c->hysteresis  = strtol(optarg, NULL, 0);         break;
            case 'O': c->offset      = strtol(optarg, NULL, 0);         break;
            case 'F': *input_stream  = open_file(optarg, "r", stdin );  break;
            case 'o': *output_stream = open_file(optarg, "w", stdout);  break;

            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *input_stream = stdin;
    FILE *output_stream = stdout;

    AUDIO_CONFIG audio = {
        .channel     = CHAN_ZERO,
        .window_size = 7,
        .threshold   = 10,
        .hysteresis  = 10,
        .offset      = 12,
    };

    if (parse_opts(&audio, argc, argv, &input_stream, &output_stream))
        exit(EXIT_FAILURE);

    // Do not buffer output at all
    setvbuf(output_stream, NULL, _IONBF, 0);

    const SERIAL_CONFIG config = {
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };

    while (true) {
        DECODE_DATA_TYPE in = 0;
        int result = fread(&in, sizeof in, 1, input_stream);

        if (result != 1) {
            if (feof(input_stream))
                break;

            perror("fread failed");
            exit(EXIT_FAILURE);
        }

        char out = 0;
        if (pump_decoder(&config, &audio, &in, &out))
            fputc(out, output_stream);
    }

    return 0;
}

