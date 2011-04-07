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

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "common.h"
#include "Viewport.h"
#include "FBO.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"

class PostProcess
{
    public:

    virtual void apply(const Viewport& viewport, 
                       Texture& rgbz_buffer,
                       Camera& camera,
                       Texture& particle_overlay) = 0;

    virtual ~PostProcess() {};
};

class SimplePostProcess : public PostProcess
{
    public:
    
    SimplePostProcess(const Viewport& viewport);

    virtual void apply(const Viewport& viewport,  
                       Texture& color_buffer,
                       Camera& camera,
                       Texture& particle_overlay);
    private: 

    Shader _post_shader;
    GPUMeshRef _screen_filling_plane;
};

class DepthOfFieldPostProcess : public PostProcess
{
    public:
    
    DepthOfFieldPostProcess(const Viewport& viewport);

    virtual ~DepthOfFieldPostProcess();

    virtual void apply(const Viewport& viewport, 
                       Texture& rgbz_buffer,
                       Camera& camera,
                       Texture& particle_overlay);

    private:

    Shader _shrink_shader;
    Shader _horz_blur_shader;
    Shader _vert_blur_shader;
    Shader _calc_coc_shader;
    Shader _pre_blur_shader;
    Shader _post_shader;

    GPUMeshRef _screen_filling_plane;

    Texture* _gauss_kernel;
    int _gauss_kernel_offset;
    
    FBO* _shrunk_fbo;
    FBO* _blurred_fbo;
    FBO* _tmp_fbo;

    vec4 _dof_lerp_scale;
    vec4 _dof_lerp_bias;
};

#endif
