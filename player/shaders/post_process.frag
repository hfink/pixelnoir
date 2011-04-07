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

uniform sampler2D color_buffer;
uniform sampler2D particle_overlay;

uniform ivec2 offset;
uniform ivec2 viewport_size;

in Varyings
{
    vec2 viewport_coord;
};

out vec4 frag_color;

float filmic_curve(float linear_color)
{
    float x = max(0, linear_color-0.004); 
    return (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06); 
}

vec4 tone_map(vec4 color)
{
    return vec4(filmic_curve(color.r),
                filmic_curve(color.g),
                filmic_curve(color.b),
                color.a);
}

void main (void)
{
    vec2 texcoord = (gl_FragCoord.xy-offset)/viewport_size;

    vec4 color = texture(color_buffer, texcoord);
    vec4 particle_value = texture(particle_overlay, texcoord);

    frag_color = tone_map(particle_value + color * (1-particle_value.a));
}
