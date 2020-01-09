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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char *argv[])
{
    if (argc != 6) {
        fputs("Supply channel, summation window size, power threshold, hysteresis, and sample offset\n", stderr);
        exit(EXIT_FAILURE);
    }

    char **arg = &argv[1];

    enum channel channel = strtol(*arg++, NULL, 0);
    int window_size = strtol(*arg++, NULL, 0);
    int threshold = strtol(*arg++, NULL, 0);
    int hysteresis = strtol(*arg++, NULL, 0);
    int offset = strtol(*arg++, NULL, 0);

    FILE *stream = stdin;

    // Do not buffer output at all
    setvbuf(stdout, NULL, _IONBF, 0);

    const SERIAL_CONFIG config = {
        .data_bits   = 7,
        .parity_bits = 1,
        .stop_bits   = 2,
    };

    while (true) {
        FILTER_IN_DATA in = 0;
        int result = fread(&in, sizeof in, 1, stream);

        if (result != 1) {
            if (feof(stream))
                break;

            perror("fread failed");
            exit(EXIT_FAILURE);
        }

        DECODE_OUT_DATA out = 0;
        if (pump_decoder(&config, channel, window_size, threshold, hysteresis, offset, in, &out))
            putchar(out);
    }

    return 0;
}

