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

#include "Viewport.h"
#include "common.h"
#include "RtrPlayerConfig.h"

Viewport* callback_viewport = NULL;

void GLFWCALL viewport_resize_callback(int width, int height)
{
    if (callback_viewport != NULL)
        callback_viewport->resize(width, height);
}

void Viewport::set_resize_callbacks() 
{
    callback_viewport = this;
    glfwSetWindowSizeCallback(viewport_resize_callback);
}


Viewport::Viewport(float aspect) :
    _aspect(aspect),
    _dynamic_aspect(aspect == 0.0f)
{
    int w,h;

    // We have to poll once to get the actual window size.
    glfwPollEvents(); 
    glfwGetWindowSize(&w, &h);
    resize(w,h);
}

void Viewport::resize(int width, int height) 
{
    if (!_dynamic_aspect) {
        
        glDisable(GL_SCISSOR_TEST);
        // Clear once to set the letterbox color;
        vec4 lb_color = config.letterbox_color();
        glClearColor(lb_color.r, lb_color.g, lb_color.b, lb_color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_SCISSOR_TEST);

        float window_aspect = float(width)/float(height);

        int w, h;

        if (_aspect < window_aspect) {
            h = height;
            w = (int)(height * _aspect);
        } else {
            w = width;
            h = (int)(width / _aspect);
        }

        _offset = ivec2((width-w)/2, (height-h)/2);
        _size = ivec2(w,h);
    } else {
        _aspect = float(width)/float(height);
        _offset = ivec2(0,0);
        _size = ivec2(width,height);
    }

    set_viewport();

    vec4 clear_color = config.clear_color();
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
}


void Viewport::set_viewport() const
{
    if(!_dynamic_aspect) {
        glScissor(_offset.x, _offset.y, _size.x, _size.y);
    }

    glViewport(_offset.x, _offset.y, _size.x, _size.y);
}

