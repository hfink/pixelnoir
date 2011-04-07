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

#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "common.h"
#include "FreeLookController.h"

class Runtime;
class Timer;

class InputHandler
{
    public:

    InputHandler(Runtime& runtime, Timer& timer);

    void set_callbacks();
    void update();
    
    void key_callback(int key, int action);

    private:

    Runtime& _runtime;
    Timer& _timer;
    ivec2 _last_mouse_pos;
    FreeLookController _render_cam_freelook;
    FreeLookController _cull_cam_freelook;
    FreeLookController * _active_freelook;
    FreeLookController * _inactive_freelook;
    bool _control_camera;
};

#endif
