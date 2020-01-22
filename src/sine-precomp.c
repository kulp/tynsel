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

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif

static const SINE_TABLE_TYPE PROGMEM generated_sines[WAVE_TABLE_SIZE] =
    #include STR(CAT(CAT(sinetable_,WAVE_TABLE_SIZE),CAT(CAT(_,ENCODE_BITS),b.h)))
;
void CAT(init_sines,ENCODE_BITS)(const void **sines, float gain)
{
    (void)gain; // unused -- it is too late now
    *sines = &generated_sines;
}

