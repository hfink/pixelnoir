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

#ifndef DUSTPARTICLES_H
#define DUSTPARTICLES_H

#include "common.h"

#include "Camera.h"
#include "Shader.h"

class Texture;
class UniformBuffer;
class TextureArray;
class FBO;

class DustParticles : boost::noncopyable
{
    public:

    DustParticles(vec3 center, vec3 size, ivec2 framebuffer_size);
    ~DustParticles();

    void update(float time_diff, CameraRef& render_cam);
    void render(Texture& rgbz_buffer, 
                UniformBuffer& shared_UBO,
                TextureArray& shadowmaps,
                const mat4& view_projection);
    Texture& get_particle_layer();

    vec3 center() { return _center; }
    vec3 size() { return _half_size * 2.0f; }
    vec3 half_size() { return _half_size; }

    void set_center(vec3 center) { _center = center; }
    void set_size(vec3 size) { _half_size = size / 2.0f; }

    private:

    void prepare_vbo();

    vec3 _center;
    vec3 _half_size;
    Shader _shader;
    
    Texture* _particle_texture;
    FBO* _fbo;

    GLuint _vbo;
    GLuint _vao;

    /**
     * Vector of particles. 
     * xyz components contain particle position
     * w component contains particle life
     */
    vector<vec4> _particles;
};

#endif
