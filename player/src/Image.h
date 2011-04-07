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

#ifndef IMAGE_H
#define IMAGE_H

#include "common.h"

/**
 * Manages image object on the CPU side.
 */
class Image
{
    protected:

    int _dimensions; /**< The number of dimensions */
    int _width, /**< Image width */
        _height, /**< Image height */
        _depth; /**< Image depth */
    void *_data; /**< Image data */

    GLenum _format; /**< Image format */
    GLenum _type; /**< Image data type */
    GLenum _internal_format; /**< Equivalent GL texture type */

    size_t _bpp; /**< Bytes per pixel. */

    static bool devil_initialized; /**< Flag for DevIL initialization.*/

    public:

    /**
     * Create uninitialized image object
     * @param dimensions Image dimensions
     * @param w Image width
     * @param h Image height
     * @param d Image depth
     * @param format Image format
     * @param type Image data type
     * @param bpp Bytes per pixel
     */
    Image (int dimensions, int w, int h, int d,
           GLenum format, GLenum type,
           size_t bpp);

    /**
     * Copy constructor.
     * Swaps data.
     * @param image Original
     */
    Image (Image& image);

    /**
     * Construct from image file.
     * @param filename Path to image file.
     */
    Image (const string& filename);

    virtual ~Image ();

    /**
     * Save contents of image to a file.
     * @param filename Path to destination image file.
     */
    void save_to_file(const string& filename);

    int dimensions() const { return _dimensions; } /**< Get dimensions*/

    int width() const { return _width; } /**< Get width */
    int height() const { return _height; } /**< Get height */
    int depth() const { return _depth; } /**< Get depth */

    void* data() { return _data; }  /**< Get data */
    const void* const_data() const { return _data; } /**< Get data as const */ 

    GLenum format() const { return _format; } /**< Get image format */
    GLenum type() const { return _type; } /**< Get image data type */
    /** Get internal format*/
    GLenum internal_format() const { return _internal_format; }
};

#endif

