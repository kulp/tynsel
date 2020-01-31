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

#ifndef SIM_PROLOGUE_H_
#define SIM_PROLOGUE_H_

extern int sim_debug(const char *fmt, ...);

namespace wrapped_io
{
#include <avr/io.h>
}

template<typename T>
class Register
{
    volatile T& impl;

    Register(Register&) = delete;
    Register& operator=(const Register&) = delete;

public:
    Register(volatile T& store) : impl(store) {}

    Register& operator=(const T&& that)
    {
        impl = that;
        sim_debug("equal");
        return *this;
    }

    Register& operator|=(const T&& that)
    {
        impl |= that;
        sim_debug("or-equal");
        return *this;
    }

    operator const T&()
    {
        sim_debug("read");
        return impl;
    }
};

#undef DAC0
#undef DAC0_CTRLA
#undef DAC0_DATA

class DAC_t : private wrapped_io::DAC_t
{
    typedef wrapped_io::DAC_t Base;
    DAC_t(const DAC_t&) = delete;
    DAC_t& operator=(const DAC_t&) = delete;
public:
    DAC_t()
        : CTRLA(Base::CTRLA)
        , DATA(Base::DATA)
    {}

    Register<decltype(Base::CTRLA)> CTRLA;
    Register<decltype(Base::DATA)> DATA;
};

DAC_t DAC0;

#define DAC0_CTRLA  (DAC0.CTRLA)
#define DAC0_DATA   (DAC0.DATA)

using namespace wrapped_io;

#endif

