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

#include "TextureArray.h"
#include "Texture.h"
#include "RtrPlayerConfig.h"

TextureArray::TextureArray(int width, int height, int count,
                           GLenum format, GLenum internal_format,
                           GLenum mag_filter, GLenum min_filter, 
                           GLenum wrap_method) :
    _bound_unit(0), _target(GL_TEXTURE_2D_ARRAY),
    _count(count), _width(width), _height(height),
    _min_filter(min_filter), _mag_filter(mag_filter), _wrap_method(wrap_method),
    _format(format), _internal_format(internal_format)
{
    glGenTextures(1, &_texture_name);

    bind();

    if(EXTGL_EXT_texture_filter_anisotropic) {
        glTexParameterf(_target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        config.max_anisotropy());
    }

    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _mag_filter);
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _min_filter);

    glTexParameteri(_target, GL_TEXTURE_WRAP_S, _wrap_method);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, _wrap_method);

    glTexImage3D(_target,               // target
                 0,                     // level
                 _internal_format,      // internalFormat
                 _width,_height,        // size
                 _count,                // array count
                 0,                     // border
                 _format,               // format
                 GL_FLOAT,              // type
                 NULL);                 // pixels

    generate_mipmaps();                 

    unbind();
}

TextureArray::~TextureArray()
{
    assert(_bound_unit == 0);

    glDeleteTextures(1, &_texture_name);
}

void TextureArray::bind()
{
    assert (_bound_unit == 0);

    _bound_unit = Texture::unit_manager().get_unit();
    glActiveTexture(_bound_unit);
    glBindTexture(_target, _texture_name);
}

void TextureArray::unbind()
{
    assert(_bound_unit != 0);

    glActiveTexture(_bound_unit);
    glBindTexture(_target, 0);

    Texture::unit_manager().return_unit(_bound_unit);

    _bound_unit = 0;
}

void TextureArray::generate_mipmaps()
{
    assert(_bound_unit != 0);

    if (_min_filter != GL_NEAREST && _min_filter != GL_LINEAR) {
        glGenerateMipmap(_target);
    }
}
