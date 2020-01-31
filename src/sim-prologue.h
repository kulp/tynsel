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

#include <cstdint>

namespace sim
{
    typedef unsigned long long uintptr_t;
}

namespace wrapped_io
{
#include <avr/io.h>
}

extern volatile char sim_memory[];

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
        sim_debug(__PRETTY_FUNCTION__);
        return *this;
    }

    Register& operator|=(const T&& that)
    {
        impl |= that;
        sim_debug(__PRETTY_FUNCTION__);
        return *this;
    }

    operator const T&()
    {
        sim_debug(__PRETTY_FUNCTION__);
        return impl;
    }
};

namespace remapped_io
{

#define FIELD_LIST_DAC_t(_) \
    _(CTRLA) \
    _(DATA) \
    // end macro

#define INIT_MEMBERS(X)     , X(base.X)
#define DECL_REGISTER(X)    Register<decltype(base.X)> X;

class DAC_t
{
    DAC_t(const DAC_t&) = delete;
    DAC_t& operator=(const DAC_t&) = delete;

    volatile wrapped_io::DAC_t &base;
public:
    DAC_t(volatile wrapped_io::DAC_t &b)
        : base(b)
        FIELD_LIST_DAC_t(INIT_MEMBERS)
    {}

    FIELD_LIST_DAC_t(DECL_REGISTER)
};

}

extern remapped_io::DAC_t DAC0_impl;

#undef DAC0_CTRLA
#undef DAC0_DATA
#define DAC0_CTRLA  (DAC0.CTRLA)
#define DAC0_DATA   (DAC0.DATA)

using namespace wrapped_io;

#endif

