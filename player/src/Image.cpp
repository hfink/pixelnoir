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

#include "Image.h"

#include <IL/il.h>
#include "format_map.h"
#include "common_const.h"
#include <boost/regex.hpp>

bool Image::devil_initialized = false;

Image::Image(int dimensions, int w, int h, int d,
             GLenum format, GLenum type, size_t bpp) :
    _dimensions(dimensions),
    _width(w), _height(h), _depth(d),
    _format(format), _type(type), 
    _internal_format(get_internal_format(format, type)),
    _bpp(bpp)
{
    assert(dimensions >= 1 && dimensions <= 3);

    size_t size = w;
    if (dimensions >= 2) size *= h;
    if (dimensions >= 3) size *= d;

    _data = malloc(bpp * size);
}

Image::Image(Image& image) :
    _dimensions(image._dimensions),
    _width(image._width), _height(image._height), _depth(image._depth),
    _format(image._format), _type(image._type), 
    _internal_format(image._internal_format),
    _bpp(image._bpp)
{
    image._data = NULL;
}
    
#define PRINT_FIELD(field_name) cout << "\t" << #field_name << "=" << field_name << endl;

Image::Image(const string& filename) : 
    _data(NULL)
{
   
    const boost::regex normalmap_pattern(rtr::kNormalMapFormat());

    if (!devil_initialized) {
        ilInit();
        ilEnable(IL_ORIGIN_SET);
        ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
        devil_initialized = true;
    }

    ILuint il_image;
    ilGenImages(1, &il_image);
    ilBindImage(il_image);

    ILboolean success = ilLoadImage(filename.c_str());

    if (!success) {
        cerr << "Image::Image "
             << "Failed to load image file " << filename << endl;
        
        _data = NULL;
        return;
    }

    _dimensions = 2;
    _format = ilGetInteger(IL_IMAGE_FORMAT);
    if (_format == IL_LUMINANCE) {
        _format = GL_RED;
        
    }

    _type = ilGetInteger(IL_IMAGE_TYPE);
    _width = ilGetInteger(IL_IMAGE_WIDTH);
    _height = ilGetInteger(IL_IMAGE_HEIGHT);
    _depth = 0;
    _bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
    _data = malloc(_bpp * _width * _height);

    if (boost::regex_match(filename, normalmap_pattern)) {

        _internal_format = get_linear_format(_format, _type);
    } else {
        _internal_format = get_internal_format(_format, _type);
    }

    ilCopyPixels(0, 0, 0, _width, _height, 1, 
                 ilGetInteger(IL_IMAGE_FORMAT), _type, _data);

    ilBindImage(0);
    ilDeleteImages(1, &il_image);

}

void Image::save_to_file(const string& filename)
{
    if (!devil_initialized) {
        ilInit();
        devil_initialized = true;
    }

    ILuint il_image;
    ilGenImages(1, &il_image);

    ilTexImage(_width, _height, 0, _bpp, _format, _type, _data);
    ILboolean success = ilSaveImage(filename.c_str());

    if (!success) {
        cerr << "Image::Image "
             << "Failed to save to image file " 
             << filename << endl;
        
        _data = NULL;
        return;
    }    

    ilBindImage(0);
    ilDeleteImages(1, &il_image);
    
}

Image::~Image()
{
    if (_data != NULL) {
        free(_data);
    }
}
