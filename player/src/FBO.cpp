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

#include "FBO.h"
#include "Texture.h"
#include "TextureArray.h"
#include "format_map.h"

GLuint FBO::_blit_fbo = 0;

int FBOFormat::add_renderbuffer(GLenum format, GLenum attachment)
{
    Attachment a = {FBO::RENDERBUFFER, format, attachment, 
                    GL_NONE, GL_NONE, GL_NONE, 0};
    _attachments.push_back(a);

    return (int)_attachments.size()-1;
}

int FBOFormat::add_texture(GLenum format, GLenum attachment,
                           GLenum mag_filter, GLenum min_filter, 
                           GLenum wrap_method)
{
    Attachment a = {FBO::TEXTURE, format, attachment, 
                    mag_filter, min_filter, wrap_method, 0};
    _attachments.push_back(a);

    return (int)_attachments.size()-1;
}


int FBOFormat::add_texture_array(int count, GLenum format, GLenum attachment,
                                 GLenum mag_filter, GLenum min_filter, 
                                 GLenum wrap_method)
{
    Attachment a = {FBO::TEXTURE_ARRAY, format, attachment, 
                    mag_filter, min_filter, wrap_method, count};
    _attachments.push_back(a);

    return (int)_attachments.size()-1;
}

int FBOFormat::attachment_count() const
{
    return (int)_attachments.size();
}

const FBOFormat::Attachment& FBOFormat::attachment(int i) const
{
    return _attachments[i];
}

namespace 
{
    string fbo_status_name(GLenum status) {

        switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        case GL_FRAMEBUFFER_COMPLETE:
            return "GL_FRAMEBUFFER_COMPLETE";
        default:
            return "UNKNOWN";
        };
    }
}

FBO::FBO(ivec2 size, int samples, const FBOFormat& format) :
    _size(size)
{
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    
    bool valid = true;

    for (int i = 0; i < format.attachment_count(); ++i) {
        const FBOFormat::Attachment& att = format.attachment(i);

        switch (att.type) {
        case RENDERBUFFER:
            add_renderbuffer(size, samples, att.format, att.attachment);
            break;
        case TEXTURE:
            add_texture(size, samples, att.format, att.attachment,
                        att.mag_filter, att.min_filter, att.wrap_method);
            break;
        case TEXTURE_ARRAY:
            add_texture_array(size, att.texture_count, 
                              samples, att.format, att.attachment,
                              att.mag_filter, att.min_filter, att.wrap_method);
            if (att.texture_count < 1) {
                valid = false;
            }

            break;
        }

        if (att.attachment >= GL_COLOR_ATTACHMENT0 &&
            att.attachment <= GL_COLOR_ATTACHMENT0 + 31) {
            _draw_buffers.push_back(att.attachment);
        }
    }

    GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    
    
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        _complete = true;
    } else if (!valid) {
        _complete = false;
    } else {
        cerr << "Failed to create framebuffer object: "
             << fbo_status_name(status) << endl;
        _complete = false;
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    if (_blit_fbo == 0) {
        glGenFramebuffers(1, &_blit_fbo);
    }
}

FBO::~FBO()
{
    for (size_t i = 0; i < _attachments.size(); ++i) {
        const Attachment att = _attachments[i];
        switch(att.type) {
        case RENDERBUFFER:
            glDeleteRenderbuffers(1, &(att.renderbuffer));
            break;
        case TEXTURE:
            delete att.texture;
            break;
        case TEXTURE_ARRAY:
            delete att.texture_array;
            break;
        } 
    }

    glDeleteFramebuffers(1, &_fbo);
}

void FBO::add_renderbuffer(ivec2 size, int samples, 
                           GLenum format, GLenum attachment)
{
    GLuint renderbuffer;
    glGenRenderbuffers(1, &renderbuffer);

    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    if (samples == 0) {
        glRenderbufferStorage(GL_RENDERBUFFER, format, size.x, size.y);
    } else {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, 
                                         format, size.x, size.y);
    }
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment, 
                              GL_RENDERBUFFER, renderbuffer);

    Attachment a = {RENDERBUFFER, renderbuffer, NULL, NULL, attachment};
    _attachments.push_back(a);
}

void FBO::add_texture(ivec2 size, int samples, 
                      GLenum internal_format, GLenum attachment,
                      GLenum mag_filter, GLenum min_filter,
                      GLenum wrap_method)
{
    GLenum format,type;
    get_format_and_type(internal_format, &format, &type);

    Texture* texture = new Texture(2, size.x, size.y, 0, format, internal_format,
                                   mag_filter, min_filter, wrap_method, samples);

    GLenum texture_target = GL_TEXTURE_2D;
    if (samples > 0) {
        texture_target = GL_TEXTURE_2D_MULTISAMPLE;
    }
    
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, 
                           texture_target, texture->texture_name(), 0);

    Attachment a = {TEXTURE, 0, texture, NULL, attachment};
    _attachments.push_back(a);
}

void FBO::add_texture_array(ivec2 size, int count, int samples, 
                            GLenum internal_format, GLenum attachment,
                            GLenum mag_filter, GLenum min_filter,
                            GLenum wrap_method)
{
    assert(samples == 0);

    GLenum format,type;
    get_format_and_type(internal_format, &format, &type);

    TextureArray* tex_array = new TextureArray(size.x, size.y, count,
                                               format, internal_format,
                                               mag_filter, min_filter, wrap_method);

    
    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachment, 
                              tex_array->texture_name(), 0, 0);

    Attachment a = {TEXTURE_ARRAY, 0, NULL, tex_array, attachment};
    _attachments.push_back(a);
}

void FBO::set_array_index(int id, int index)
{
    assert(_attachments[id].type == TEXTURE_ARRAY);

    Attachment& att = _attachments[id];

    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, att.attachment, 
                              att.texture_array->texture_name(), 0, index);
}

void FBO::bind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    glReadBuffer(GL_NONE);
    glDrawBuffers(_draw_buffers.size(), _draw_buffers.data());
    glViewport(0,0,_size.x,_size.y);
    glScissor (0,0,_size.x,_size.y);
}

void FBO::unbind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
}

bool FBO::is_complete() const
{
    return _complete;
}

Texture& FBO::get_texture_target(int id)
{
    assert((int)_attachments.size()>id && _attachments[id].type == TEXTURE);

    return *_attachments[id].texture;
}


TextureArray& FBO::get_texture_array(int id)
{
    assert((int)_attachments.size()>id && _attachments[id].type == TEXTURE_ARRAY);

    return *_attachments[id].texture_array;
}

void FBO::blit(int id, Texture& tex,
               ivec2 src_offset, ivec2 src_size,
               ivec2 dst_offset, ivec2 dst_size,
               GLenum filter)
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _blit_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
    
    GLenum source = _attachments[id].attachment;
    GLenum destination = GL_COLOR_ATTACHMENT0;

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, destination,
                           GL_TEXTURE_2D, tex.texture_name(), 0);

    glReadBuffer(source);
    glDrawBuffers(1, &destination);

    glBlitFramebuffer(src_offset.x, src_offset.y, src_size.x, src_size.y,
                      dst_offset.x, dst_offset.y, dst_size.x, dst_size.y,
                      GL_COLOR_BUFFER_BIT, filter);
                      

    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, destination,
                           GL_TEXTURE_2D, 0, 0);    

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

ivec2 FBO::get_size()
{
    return _size;
}
