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

#ifndef FBO_H
#define FBO_H

#include "common.h"

class Texture;
class TextureArray;
class FBOFormat;

class FBO : boost::noncopyable
{
    public:
    
    enum AttachmentType
    {
        RENDERBUFFER,
        TEXTURE,
        TEXTURE_ARRAY
    };

    FBO(ivec2 size, int samples, const FBOFormat& format);
    ~FBO();

    void bind();
    void unbind();

    void set_array_index(int id, int index);

    bool is_complete() const;

    ivec2 get_size();

    Texture& get_texture_target(int id);
    TextureArray& get_texture_array(int id);

    void blit(int id, Texture& tex,
              ivec2 src_offset, ivec2 src_size,
              ivec2 dst_offset, ivec2 dst_size,
              GLenum filter = GL_NEAREST);
    
    private:

    void add_renderbuffer(ivec2 size, int samples, 
                          GLenum format, GLenum attachment);

    void add_texture(ivec2 size, int samples, 
                     GLenum internal_format, GLenum attachment,
                     GLenum mag_filter,GLenum min_filter,
                     GLenum wrap_method);   

    void add_texture_array(ivec2 size, int count, int samples, 
                           GLenum internal_format, GLenum attachment,
                           GLenum mag_filter,GLenum min_filter,
                           GLenum wrap_method);   

    struct Attachment
    {
        AttachmentType type;
        GLuint renderbuffer;
        Texture* texture;
        TextureArray* texture_array;
        GLenum attachment;
    };

    ivec2 _size;
    GLuint _fbo;
    vector<GLenum> _draw_buffers;
    vector<Attachment> _attachments;
    bool _complete;

    static GLuint _blit_fbo;
};

class FBOFormat
{
    public:

    struct Attachment
    {
        FBO::AttachmentType type;
        GLenum format;
        GLenum attachment;
        GLenum mag_filter;
        GLenum min_filter;
        GLenum wrap_method;
        int texture_count;
    };

    int add_renderbuffer(GLenum format, GLenum attachment);
    int add_texture(GLenum format, GLenum attachment,
                    GLenum mag_filter = GL_NEAREST,
                    GLenum min_filter = GL_NEAREST,
                    GLenum wrap_method = GL_CLAMP_TO_EDGE);
    int add_texture_array(int count, GLenum format, GLenum attachment,
                          GLenum mag_filter = GL_NEAREST,
                          GLenum min_filter = GL_NEAREST,
                          GLenum wrap_method = GL_CLAMP_TO_EDGE);

    int attachment_count() const;
    const Attachment& attachment(int i) const;

    private:

    vector<Attachment>  _attachments;
};

#endif
