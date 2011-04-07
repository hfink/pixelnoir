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

#version 330

uniform sampler2DArray source;
uniform vec2 source_size_inv;
uniform sampler1D kernel;

uniform int kernel_width;

uniform int offset;
uniform int texture_index;

in Varyings
{
    vec2 tex_coord;
};

out vec4 frag_color;

vec4 gather_samples(void)
{
    vec2 inc = vec2(source_size_inv.x, 0);
    
    vec2 pos = tex_coord + inc * offset;

    vec4 color = vec4(0);
    float sum = 0;

    for (int i = 0; i < kernel_width; ++i) {
        float f = texelFetch(kernel, i, 0).r;
        vec4 c = texture(source, vec3(pos + inc * i, texture_index));
        if (!any(isnan(c))) {
            color += f * c;
            sum += f;
        }
    }

    return color / sum;
}

void main (void)
{
    frag_color = gather_samples();
}