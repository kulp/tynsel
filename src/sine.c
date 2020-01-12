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

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

#define STR(X) STR_(X)
#define STR_(X) # X

#define int16_tMAX INT16_MAX
#define  int8_tMAX  INT8_MAX

#include <math.h>
#include <stddef.h>

// Creates a quarter-wave sine table, scaled by the given gain.
// Valid indices into the table are [0,WAVE_TABLE_SIZE).
// Input indices are augmented by 0.5 before computing the sine, on the
// assumption that WAVE_TABLE_SIZE is a power of two and that we want to do
// arithmetic with powers of two. If we did not compensate somehow, we would
// either have two entries for zero (when flipping the quadrant), two entries
// for max (also during flipping), or an uneven gap between the minimum
// positive and minimum negative output values.
void make_sine_table(size_t size, SINE_TABLE_TYPE sines[size], float gain)
{
    const SINE_TABLE_TYPE max = (CAT(SINE_TABLE_TYPE,MAX) / 2);
    for (unsigned int i = 0; i < size; i++) {
        sines[i] = (SINE_TABLE_TYPE)lroundf(gain * sinf(2 * M_PI * (i + 0.5) / (size * 4)) * max - 1);
    }
}

void init_sines(void **sines, float gain)
{
    static SINE_TABLE_TYPE private_sines[WAVE_TABLE_SIZE];
    make_sine_table(WAVE_TABLE_SIZE, private_sines, gain);
    *sines = &private_sines;
}

