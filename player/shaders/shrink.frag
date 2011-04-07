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

#version 400

uniform sampler2D rgbz_buffer;
uniform vec4 offset;

uniform vec4 dof_world;

in Varyings
{
    vec2 tex_coord;
};

out vec4 frag_color;

vec4 calcCoC(vec4 depths)
{
    return clamp(dof_world.x * depths + vec4(dof_world.y), 0, 1);
}

vec4 gather_samples(void)
{

    vec3 color = vec3(0);
    color += texture(rgbz_buffer, tex_coord + offset.xy).rgb;
    color += texture(rgbz_buffer, tex_coord + offset.zy).rgb;
    color += texture(rgbz_buffer, tex_coord + offset.xw).rgb;
    color += texture(rgbz_buffer, tex_coord + offset.zw).rgb;

    vec4 depths, curCoC;

    depths = textureGather(rgbz_buffer, tex_coord + offset.xy, 3);
    curCoC = calcCoC(depths);

    depths = textureGather(rgbz_buffer, tex_coord + offset.zy, 3);
    curCoC = max(curCoC, calcCoC(depths));

    depths = textureGather(rgbz_buffer, tex_coord + offset.xw, 3);
    curCoC = max(curCoC, calcCoC(depths));

    depths = textureGather(rgbz_buffer, tex_coord + offset.zw, 3);
    curCoC = max(curCoC, calcCoC(depths));

    float maxCoC = max(max(curCoC.x, curCoC.y), max(curCoC.z, curCoC.w));

    return vec4(color * 0.25, maxCoC);
}

void main (void)
{
    frag_color = gather_samples();
}
