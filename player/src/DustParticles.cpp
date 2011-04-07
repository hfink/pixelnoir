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

#include "DustParticles.h"

#include "Camera.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "TextureArray.h"
#include "FBO.h"

#include "RtrPlayerConfig.h"
#include "Image.h"

#include <glm/gtx/random.hpp>

DustParticles::DustParticles(vec3 center, vec3 size, ivec2 framebuffer_size) :
    _center(center),
    _half_size(size * 0.5f),
    _shader("dust")
{
    FBOFormat format;
    format.add_texture(GL_RGBA16F, GL_COLOR_ATTACHMENT0, 
                       GL_LINEAR, GL_LINEAR);

    _fbo = new FBO(framebuffer_size, 0, format);

    _particles.resize(config.dust_particle_count(), vec4(0));

    prepare_vbo();

    Image image(config.texture_dir()+"/dust_particle_nm.png");
    _particle_texture = new Texture(image);
}

DustParticles::~DustParticles()
{
    delete _particle_texture;
    delete _fbo;
}

void DustParticles::update(float time_diff, CameraRef& render_cam)
{
    float life_range = config.dust_max_life() - config.dust_min_life();
    
    float top = _center.z + _half_size.z;
    float bottom = _center.z - _half_size.z;

    for (size_t i = 0; i < _particles.size(); ++i) {
        vec4& particle = _particles[i];

        if (particle.w <= 0.0f) {
            // generate new particle

            vec3 pos = glm::compRand3(-1.0f, 1.0f) * _half_size + _center;
            float life = glm::compRand1(0.0f, 1.0f) * life_range + config.dust_min_life();

            particle = vec4(pos, life);            
        } else {
            // Update particle position
            
            vec3 pos(particle.x, particle.y, particle.z);

            pos += vec3(0,0,-1.0f) * time_diff * config.dust_particle_speed();
            if (pos.z < bottom) pos.z = top;

            particle = vec4(pos, particle.w);

            //particle.w -= time_diff;
        }
    }

    // Send to VBO
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _particles.size() * sizeof(vec4),
                 _particles.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DustParticles::render(Texture& rgbz_buffer,
                           UniformBuffer& shared_UBO,
                           TextureArray& shadowmaps,
                           const mat4& view_projection)
{
    _fbo->bind();
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Use premultiplied alpha blending
    glEnablei(GL_BLEND, 0);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);    

    rgbz_buffer.bind();
    shadowmaps.bind();
    _particle_texture->bind();
    
    shared_UBO.bind();

    _shader.bind();

    _shader.set_uniform_block("Shared", shared_UBO);
    _shader.set_uniform("shadowmaps", shadowmaps);
    _shader.set_uniform("view_projection", view_projection);
    _shader.set_uniform("pixel_size", vec2(1.0/rgbz_buffer.width(),
                                           1.0/rgbz_buffer.height()));
    _shader.set_uniform("rgbz_buffer", rgbz_buffer);
    _shader.set_uniform("particle_texture", *_particle_texture);
    _shader.set_uniform("size_factor", config.dust_size_factor());
    _shader.set_uniform("light_factor", config.dust_light_factor());
    _shader.set_uniform("light_exponent", config.dust_light_exponent());
    _shader.set_uniform("particle_alpha", config.dust_particle_alpha());


    glBindVertexArray(_vao);

    glDrawArrays(GL_POINTS, 0, _particles.size());

    glBindVertexArray(0);

    _shader.unbind();

    shared_UBO.unbind();

    _particle_texture->unbind();
    shadowmaps.unbind();
    rgbz_buffer.unbind();

    _fbo->unbind();
    glDisablei(GL_BLEND, 0);
}

Texture& DustParticles::get_particle_layer()
{
    return _fbo->get_texture_target(0);
}

void DustParticles::prepare_vbo()
{
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    GLint vertex_attrib_location = _shader.get_attrib_location("vertex");
    
    // Create and bind VBO
    
    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _particles.size() * sizeof(vec4), 
                 NULL, GL_STREAM_DRAW);

    glEnableVertexAttribArray(vertex_attrib_location);
    glVertexAttribPointer(vertex_attrib_location, 4, 
                          GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glBindVertexArray(0);
}
