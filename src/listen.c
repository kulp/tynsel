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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *open_file(const char *filename, const char *mode, FILE *dflt)
{
    if (strcmp(filename, "-") == 0)
        return dflt;

    return fopen(filename, mode);
}

static int parse_opts(AUDIO_CONFIG *c, int argc, char *argv[], uint8_t *bits, FILE **input_stream, FILE **output_stream)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:W:T:H:O:b:F:o:")) != -1) {
        switch (ch) {
            case 'C': c->channel     = strtol(optarg, NULL, 0);         break;
            case 'W': c->window_size = strtol(optarg, NULL, 0);         break;
            case 'T': c->threshold   = strtol(optarg, NULL, 0);         break;
            case 'H': c->hysteresis  = strtol(optarg, NULL, 0);         break;
            case 'O': c->offset      = strtol(optarg, NULL, 0);         break;
            case 'b': *bits          = strtol(optarg, NULL, 0);         break;
            case 'F': *input_stream  = open_file(optarg, "r", stdin );  break;
            case 'o': *output_stream = open_file(optarg, "w", stdout);  break;

            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

decode_init make_decode_state8;
decode_init make_decode_state16;

decode_pumper pump_decoder8;
decode_pumper pump_decoder16;

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

    uint8_t bits = 16;
    if (parse_opts(&audio, argc, argv, &bits, &input_stream, &output_stream))
        exit(EXIT_FAILURE);

    struct {
        decode_init *init;
        decode_pumper *pump;
    } decoders[] = {
        [8]  = { make_decode_state8,  pump_decoder8  },
        [16] = { make_decode_state16, pump_decoder16 },
    };

    if (bits >= sizeof(decoders) / sizeof(decoders[0]) || ! decoders[bits].init) {
        fprintf(stderr, "No decoder found for bits=%d\n", bits);
        exit(EXIT_FAILURE);
    }

    decode_init   *init_decoder = decoders[bits].init;
    decode_pumper *pump_decoder = decoders[bits].pump;

    DECODE_STATE *state = init_decoder();

    // Do not buffer output at all
    setvbuf(output_stream, NULL, _IONBF, 0);

    const SERIAL_CONFIG config = {
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };

    while (true) {
        int in = 0;
        int result = fread(&in, bits / CHAR_BIT, 1, input_stream);

        if (result != 1) {
            if (feof(input_stream))
                break;

            perror("fread failed");
            exit(EXIT_FAILURE);
        }

        char out = 0;
        if (pump_decoder(&config, &audio, state, &in, &out))
            fputc(out, output_stream);
    }

    return 0;
}

