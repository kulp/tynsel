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

#ifndef COEFF_H_
#define COEFF_H_

#include "types.h"

#define COEFF_FRACTIONAL_BITS 14

#if defined(USE_FLOATING_POINT)
typedef float FILTER_COEFF;
typedef float FILTER_STATE_DATA;
#define DEFINE_COEFF(x) (x)
#define FILTER_MULT(a, b) ((a) * (b))
#else
typedef int16_t FILTER_COEFF;
typedef int16_t FILTER_STATE_DATA;
#define DEFINE_COEFF(x) ((FILTER_COEFF)((x) * (1 << COEFF_FRACTIONAL_BITS)))
#define FILTER_MULT(a, b) (((a) * (b)) >> COEFF_FRACTIONAL_BITS)
#endif

struct filter_config {
    FILTER_COEFF coeff_b0, coeff_b1, coeff_a2;
#define coeff_b2 coeff_b0
#define coeff_a1 coeff_b1
};

extern const struct filter_config coeff_table[];

#endif

