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

#include "PostProcess.h"
#include "mesh_generation.h"
#include "RtrPlayerConfig.h"
#include "Image.h"

SimplePostProcess::SimplePostProcess(const Viewport& viewport) : 
    _post_shader("post_process")                  
{
    LayerSourceInitializerMap lyr_init_map;
    MeshInitializer mesh = create_screen_filling_plane(lyr_init_map);

    GPULayerSourceMap gpu_lyr_map = 
        LayerSourceInitializer::to_gpu_layer_map(lyr_init_map);

    _screen_filling_plane = GPUMeshRef(new GPUMesh(mesh, gpu_lyr_map));
}

void SimplePostProcess::apply(const Viewport& viewport, 
                              Texture& color_buffer,
                              Camera& camera,
                              Texture& particle_overlay)
{
    glDisable(GL_DEPTH_TEST);
    viewport.set_viewport();

    _post_shader.bind();

    color_buffer.bind();
    particle_overlay.bind();
    
    _post_shader.set_uniform("color_buffer", color_buffer);
    _post_shader.set_uniform("offset", viewport.render_pixel_offset());
    _post_shader.set_uniform("viewport_size", viewport.render_size());
    _post_shader.set_uniform("particle_overlay", particle_overlay);

    _screen_filling_plane->draw(_post_shader);

    color_buffer.unbind();
    particle_overlay.unbind();
    _post_shader.unbind();
}

DepthOfFieldPostProcess::DepthOfFieldPostProcess(const Viewport& viewport) :
    _shrink_shader(EXTGL_MAJOR_VERSION >= 4?"shrink":"shrink_fallback"),
    _horz_blur_shader("horz_blur_shader"),
    _vert_blur_shader("vert_blur_shader"),
    _calc_coc_shader("calc_coc"),
    _pre_blur_shader("pre_blur"),
    _post_shader("post_process_dof")
{
    LayerSourceInitializerMap lyr_init_map;
    MeshInitializer mesh = create_screen_filling_plane(lyr_init_map);

    GPULayerSourceMap gpu_lyr_map = 
        LayerSourceInitializer::to_gpu_layer_map(lyr_init_map);

    _screen_filling_plane = GPUMeshRef(new GPUMesh(mesh, gpu_lyr_map));

    FBOFormat format;
    format.add_texture(GL_RGBA16F, GL_COLOR_ATTACHMENT0, 
                       GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

    ivec2 size = viewport.render_size() / 4;
    
    _shrunk_fbo = new FBO(size, 0, format);
    _blurred_fbo = new FBO(size, 0, format);
    _tmp_fbo = new FBO(size, 0, format);

    int radius = int(config.dof_blur_radius() / 150. * size.y)+1;
    int L = 1 + radius * 2;
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
    
    ivec2 rsize = viewport.render_size();

    float d0 = 5 / (L * 4.0f);
    float d1 = (3 * 4) / (L * 4.0f) - d0;
    float d2 = 1 - d1 - d0;

    assert(d0 > 0 && d0 <=1 &&
           d1 > 0 && d1 <=1 &&
           d2 > 0 && d2 <=1 &&
           d0 + d1 + d2 > 0.999 && d0 + d1 + d2 < 1.001);

    _dof_lerp_scale = vec4( -1 / d0, -1 / d1, -1 / d2, 1 / d2);
    _dof_lerp_bias  = vec4( 1, (1 - d2) / d1, 1 / d2, (d2 - 1) / d2);
}

DepthOfFieldPostProcess::~DepthOfFieldPostProcess()
{
    delete _gauss_kernel;

    delete _shrunk_fbo;
    delete _blurred_fbo;
    delete _tmp_fbo;
}

void DepthOfFieldPostProcess::apply(const Viewport& viewport,
                                    Texture& rgbz_buffer,
                                    Camera& camera,
                                    Texture& particle_overlay)
{
    glDisable(GL_DEPTH_TEST);

    float focus_depth = camera.calculate_focus_depth();

    vec4 d = config.dof_depth_range_100() * focus_depth / 100.0f;
    vec4 dof_world( 1/(d[0]-d[1]), -d[1]/(d[0]-d[1]), 
                   -1/(d[2]-d[3]),  d[2]/(d[2]-d[3]));

    // Shrink RGBZ buffer -----------------------------------------------
    
    _shrunk_fbo->bind();
    _shrink_shader.bind();

    rgbz_buffer.bind();

    _shrink_shader.set_uniform("rgbz_buffer", rgbz_buffer);
    _shrink_shader.set_uniform("dof_world", dof_world);
    _shrink_shader.set_uniform("offset", 
                               vec4(-1.0f/rgbz_buffer.width(), 
                                    -1.0f/rgbz_buffer.height(),
                                     1.0f/rgbz_buffer.width(), 
                                     1.0f/rgbz_buffer.height()));

    _screen_filling_plane->draw(_shrink_shader);

    rgbz_buffer.unbind();

    _shrink_shader.unbind();
    _shrunk_fbo->unbind();

    Texture& shrunk_buffer = _shrunk_fbo->get_texture_target(0);

    // Horizontal gaussian blur ----------------------------------------

    _gauss_kernel->bind();

    _tmp_fbo->bind();
    _horz_blur_shader.bind();

    shrunk_buffer.bind();

    _horz_blur_shader.set_uniform("source", shrunk_buffer);
    _horz_blur_shader.set_uniform("source_size_inv", 
                                  vec2(1.0f/shrunk_buffer.width(),
                                       1.0f/shrunk_buffer.height()));
    _horz_blur_shader.set_uniform("kernel", *_gauss_kernel);
    _horz_blur_shader.set_uniform("kernel_width", _gauss_kernel->width());
    _horz_blur_shader.set_uniform("offset", _gauss_kernel_offset);

    _screen_filling_plane->draw(_horz_blur_shader);

    shrunk_buffer.unbind();

    _horz_blur_shader.unbind();
    _tmp_fbo->unbind();


    Texture& tmp_buffer = _tmp_fbo->get_texture_target(0);

    // Vertical gaussian blur -------------------------------------------

    _blurred_fbo->bind();
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
    _blurred_fbo->unbind();

    Texture& blurred_buffer = _blurred_fbo->get_texture_target(0);

    // Calculate actual Near CoC ---------------------------------------

    _tmp_fbo->bind();
    _calc_coc_shader.bind();

    shrunk_buffer.bind();
    blurred_buffer.bind();

    _calc_coc_shader.set_uniform("shrunk_buffer", shrunk_buffer);
    _calc_coc_shader.set_uniform("blurred_buffer", blurred_buffer);

    _screen_filling_plane->draw(_calc_coc_shader);

    blurred_buffer.unbind();
    shrunk_buffer.unbind();

    _calc_coc_shader.unbind();
    _tmp_fbo->unbind();

    // Pre-blur shrunk buffer one last time -\--------------------------

    _shrunk_fbo->bind();

    _pre_blur_shader.bind();
    tmp_buffer.bind();

    _pre_blur_shader.set_uniform("source", tmp_buffer);
    _pre_blur_shader.set_uniform("offset", 
                                  vec4(-0.5f/tmp_buffer.width(),
                                       -0.5f/tmp_buffer.height(),
                                        0.5f/tmp_buffer.width(),
                                        0.5f/tmp_buffer.height()));

    _screen_filling_plane->draw(_pre_blur_shader);

    tmp_buffer.unbind();
    _pre_blur_shader.unbind();
    
    _shrunk_fbo->unbind();

    // Final Post-Process pass -----------------------------------------

    _gauss_kernel->unbind();

    viewport.set_viewport();

    _post_shader.bind();

    rgbz_buffer.bind();
    shrunk_buffer.bind();
    blurred_buffer.bind();
    particle_overlay.bind();
    
    _post_shader.set_uniform("rgbz_buffer", rgbz_buffer);
    _post_shader.set_uniform("shrunk_buffer", shrunk_buffer);
    _post_shader.set_uniform("rgbz_buffer_size_inv", 
                            vec2(1.0f/rgbz_buffer.width(),
                                 1.0f/rgbz_buffer.height()));
    _post_shader.set_uniform("shrunk_buffer_size_inv", 
                            vec2(1.0f/shrunk_buffer.width(),
                                 1.0f/shrunk_buffer.height()));
    _post_shader.set_uniform("blurred_buffer", blurred_buffer);
    _post_shader.set_uniform("dof_lerp_scale", _dof_lerp_scale);
    _post_shader.set_uniform("dof_lerp_bias", _dof_lerp_bias);
    _post_shader.set_uniform("dof_world", dof_world);
    _post_shader.set_uniform("far_coc_radius", config.dof_far_radius());
    _post_shader.set_uniform("particle_overlay", particle_overlay);
    _post_shader.set_uniform("barrel_distortion", config.barrel_distortion());

    _screen_filling_plane->draw(_post_shader);

    rgbz_buffer.unbind();
    shrunk_buffer.unbind();
    blurred_buffer.unbind();
    particle_overlay.unbind();
    _post_shader.unbind();    
}

