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

#ifndef TEXTUREARRAY_H
#define TEXTUREARRAY_H

#include "common.h"

class TextureArray : boost::noncopyable
{
    GLenum _bound_unit;
    GLenum _target;
    GLuint _texture_name;

    int _count;
    int _width, _height;

    GLenum _min_filter;
    GLenum _mag_filter;
    GLenum _wrap_method;

    GLenum _format;
    GLenum _internal_format;

    public:

    TextureArray(int width, int height, int count,
                 GLenum format, GLenum internal_format,
                 GLenum mag_filter = GL_NEAREST, GLenum min_filter = GL_NEAREST,
                 GLenum wrap_method = GL_CLAMP_TO_EDGE);

    ~TextureArray();

    void bind();
    void unbind();

    GLint get_unit_number() const { return _bound_unit - GL_TEXTURE0; }
    GLint texture_name() const { return _texture_name; }

    bool is_bound() const { return _bound_unit != 0; }

    int texture_count() const { return _count; }

    void generate_mipmaps();
};

#endif
