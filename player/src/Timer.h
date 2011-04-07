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

#ifndef TIMER_H
#define TIMER_H

class Timer
{
    double _real_now;
    double _real_diff;
    
    double _now;
    double _diff;

    double _factor;

    bool _paused;

    public:

    Timer(double now) :
        _real_now(now), _real_diff(0.0),
        _now(0.0), _diff(0.0), _factor(1.0),
        _paused(false) {};

    void update(double now);

    void update_diff(double diff);

    void pause();
    void play();
    void rewind();

    void forward();
    void slow_forward();
    void fast_forward();
    
    void reverse();
    void slow_reverse();
    void fast_reverse();

    bool paused () const { return _paused; }

    double now () const { return _now ; }
    double diff() const { return _diff; }
    double real_diff() const { return _real_diff; }
};

#endif
