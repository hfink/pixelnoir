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

#include "GaussianBlur.h"

#include "FBO.h"
#include "Texture.h"

#include "TextureArray.h"
#include "Image.h"
#include "mesh_generation.h"

GaussianBlur::GaussianBlur(int kernel_radius,
                           ivec2 texture_size, GLenum internal_format) :
    _horz_blur_shader("horz_blur_array_shader"),
    _vert_blur_shader("vert_blur_shader")
{
    int L = 1 + kernel_radius * 2;
    int N = L - 1;

    Image kernel_image(1, L, 0, 0, GL_RED, GL_FLOAT, 32);
    float* kernel = (float*) kernel_image.data();
    float alpha = 2.5;

    _gauss_kernel_offset = -N/2;

    float sum = 0.0f;
    for (int n = -N/2; n <= N/2; ++n) {
        kernel[n-_gauss_kernel_offset] = expf(-0.5f * powf((alpha * n)/(N/2), 2.0f));
        sum += kernel[n-_gauss_kernel_offset];
    }

    for (int i = 0; i < L; ++i) {
        kernel[i] /= sum;
    }

    _gauss_kernel = new Texture(kernel_image, 
                                GL_NEAREST, GL_NEAREST, 
                                GL_CLAMP_TO_EDGE);


    FBOFormat format;
    format.add_texture(internal_format, GL_COLOR_ATTACHMENT0,
                       GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);

    _tmp_fbo = new FBO(texture_size, 0, format);

    LayerSourceInitializerMap lyr_init_map;
    MeshInitializer mesh = create_screen_filling_plane(lyr_init_map);
    
    GPULayerSourceMap sources = 
        LayerSourceInitializer::to_gpu_layer_map(lyr_init_map);

    _screen_filling_plane = GPUMeshRef(new GPUMesh(mesh, sources));
}

GaussianBlur::~GaussianBlur()
{
    delete _tmp_fbo;
    delete _gauss_kernel;
}

void GaussianBlur::blur_texture_array(FBO& fbo, int attachment_id)
{
    glDisable(GL_DEPTH_TEST);

    TextureArray& texture = fbo.get_texture_array(attachment_id);

    _gauss_kernel->bind();
    for (int i = 0; i < texture.texture_count(); ++i) {
        // Horizontal gaussian blur ----------------------------------------

        _tmp_fbo->bind();
        _horz_blur_shader.bind();

        texture.bind();

        _horz_blur_shader.set_uniform("source", texture);
        _horz_blur_shader.set_uniform("source_size_inv", 
                                      vec2(1.0f/fbo.get_size().x,
                                           1.0f/fbo.get_size().y));
        _horz_blur_shader.set_uniform("kernel", *_gauss_kernel);
        _horz_blur_shader.set_uniform("kernel_width", _gauss_kernel->width());
        _horz_blur_shader.set_uniform("offset", _gauss_kernel_offset);
        _horz_blur_shader.set_uniform("texture_index", i);

        _screen_filling_plane->draw(_horz_blur_shader);

        texture.unbind();

        _horz_blur_shader.unbind();
        _tmp_fbo->unbind();


        Texture& tmp_buffer = _tmp_fbo->get_texture_target(0);

        // Vertical gaussian blur -------------------------------------------

        fbo.bind();
        fbo.set_array_index(attachment_id, i);

        _vert_blur_shader.bind();

        tmp_buffer.bind();

        _vert_blur_shader.set_uniform("source", tmp_buffer);
        _vert_blur_shader.set_uniform("source_size_inv", 
                                      vec2(1.0f/tmp_buffer.width(),
                                           1.0f/tmp_buffer.height()));
        _vert_blur_shader.set_uniform("kernel", *_gauss_kernel);
        _vert_blur_shader.set_uniform("kernel_width", _gauss_kernel->width());
        _vert_blur_shader.set_uniform("offset", _gauss_kernel_offset);

        _screen_filling_plane->draw(_vert_blur_shader);

        tmp_buffer.unbind();

        _vert_blur_shader.unbind();
        fbo.unbind();
    }

    _gauss_kernel->unbind();

    glEnable(GL_DEPTH_TEST);
}
