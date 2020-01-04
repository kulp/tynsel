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

#include <math.h>

#define int16_tMAX INT16_MAX
#define  int8_tMAX  INT8_MAX

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

// Creates a quarter-wave sine table, scaled by the given gain.
// Valid indices into the table are [0,WAVE_TABLE_SIZE).
// Input indices are augmented by 0.5 before computing the sine, on the
// assumption that WAVE_TABLE_SIZE is a power of two and that we want to do
// arithmetic with powers of two. If we did not compensate somehow, we would
// either have two entries for zero (when flipping the quadrant), two entries
// for max (also during flipping), or an uneven gap between the minimum
// positive and minimum negative output values.
void make_sine_table(ENCODE_DATA_TYPE sines[WAVE_TABLE_SIZE], float gain)
{
    const ENCODE_DATA_TYPE max = (CAT(ENCODE_DATA_TYPE,MAX) / 2);
    for (unsigned int i = 0; i < WAVE_TABLE_SIZE; i++) {
        sines[i] = (ENCODE_DATA_TYPE)(gain * sinf(2 * M_PI * (i + 0.5) / MAJOR_PER_CYCLE) * max - 1);
    }
}

