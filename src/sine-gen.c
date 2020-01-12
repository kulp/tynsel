/*
 * Copyright (c) 2020 Darren Kulp
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

#include "sine.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    extern void CAT(make_sine_table,ENCODE_BITS)(size_t size, SINE_TABLE_TYPE sines[size], float gain);

    if (argc != 3)
        exit(EXIT_FAILURE);

    size_t size = strtoul(argv[1], NULL, 0);
    float gain = strtof(argv[2], NULL);

    SINE_TABLE_TYPE sines[size];
    CAT(make_sine_table,ENCODE_BITS)(size, sines, gain);

    puts("{");
    for (size_t i = 0; i < size; i++)
        printf("    %u,\n", sines[i]);
    puts("}");
}

