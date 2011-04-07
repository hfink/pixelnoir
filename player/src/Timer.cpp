//Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
//                   Thomas Weber <weber (dot) t (at) gmx (dot) at>
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

#include "Timer.h"

void Timer::update(double now)
{
    double diff = now - _real_now;
    _real_now = now;

    _real_diff = diff;

    if (!_paused) {
        _diff = diff * _factor;
        _now += diff * _factor;
    } else {
        _diff = 0.0;
    }
}

void Timer::update_diff(double diff)
{
    _real_now += diff;

    _real_diff = diff;

    if (!_paused) {
        _diff = diff * _factor;
        _now += diff * _factor;
    } else {
        _diff = 0.0;
    }
}

void Timer::pause()
{
    _paused = true;
}

void Timer::play()
{
    _paused = false;
}

void Timer::rewind()
{
    _now = 0.0;
}

void Timer::forward()
{
    _factor = 1.0;
}

void Timer::slow_forward()
{
    _factor = 0.1;
}

void Timer::fast_forward()
{
    _factor = 10.0;
}


void Timer::reverse()
{
    _factor = -1.0;
}

void Timer::slow_reverse()
{
    _factor = -0.1;
}

void Timer::fast_reverse()
{
    _factor = -10.0;
}


