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

@include <shared.glsl>
@include <eval_light.glsl>

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
    
uniform vec2 pixel_size;

uniform float size_factor;
uniform float light_factor;
uniform float light_exponent;
uniform float particle_alpha;

in vec2 screen_position[];
in float depth[];
in vec3 world_position[];

out vec2 tex_coord;
out float alpha;
out vec3 lighting;

uniform sampler2D rgbz_buffer;

vec3 gather_lighting (vec3 position)
{
    vec3 total = vec3(0.0);
    for (int i = 0; i < light_count; ++i) {
        vec3 intensity, light_dir;

        eval_light(i, position, light_dir, intensity);

        total += intensity;
    }
    
    return total;
}

void main(void)
{
    if (depth[0] <= 0) return;

    vec2 tc = screen_position[0] * 0.5 + vec2(0.5);

    float rgbz_depth = texture(rgbz_buffer, tc).a;

    if (depth[0] > rgbz_depth) return;

    vec2 offset = pixel_size * max(2, size_factor / depth[0]);

    lighting = pow(gather_lighting(world_position[0]),
                   vec3(light_exponent)) * light_factor;
    alpha = particle_alpha;
    gl_Position.zw = vec2(0,1);

    gl_Position.xy = screen_position[0] + offset * vec2(-1,-1);
    tex_coord = vec2(0,0);
    EmitVertex();

    gl_Position.xy = screen_position[0] + offset * vec2( 1,-1);
    tex_coord = vec2(1,0);
    EmitVertex();

    gl_Position.xy = screen_position[0] + offset * vec2(-1, 1);
    tex_coord = vec2(0,1);
    EmitVertex();

    gl_Position.xy = screen_position[0] + offset * vec2( 1, 1);
    tex_coord = vec2(1,1);
    EmitVertex();

    EndPrimitive();
}
