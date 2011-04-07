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

uniform sampler2D rgbz_buffer;

uniform sampler2D shrunk_buffer;
uniform sampler2D blurred_buffer;

uniform vec4 dof_lerp_scale;
uniform vec4 dof_lerp_bias;
uniform vec4 dof_world;
uniform float far_coc_radius;

uniform vec2 rgbz_buffer_size_inv;
uniform vec2 shrunk_buffer_size_inv;

uniform sampler2D particle_overlay;

uniform float barrel_distortion;

in Varyings
{
    vec2 screen_pos;
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

vec4 get_offset_sample(vec2 pos, vec2 offset, vec2 tex_coord)
{
    return texture(rgbz_buffer, tex_coord + offset * rgbz_buffer_size_inv.xy);
}

vec4 get_small_blur(vec2 tex_coord)
{
    vec4 sum = vec4(0);

    sum += get_offset_sample(tex_coord, vec2(+0.5, -1.5), tex_coord);
    sum += get_offset_sample(tex_coord, vec2(-1.5, -0.5), tex_coord);
    sum += get_offset_sample(tex_coord, vec2(-0.5, +1.5), tex_coord);
    sum += get_offset_sample(tex_coord, vec2(+1.5, +0.5), tex_coord);

    return sum * (4.0 / 17.0);
}

vec4 bezier_weights (float x)
{
    const mat4 MB = mat4( 1, 4, 2, 0,
                         -3, 0, 3, 0,
                          3,-6, 3, 0,
                         -1, 3,-3, 1);

    vec4 alpha = vec4(1, x, x*x*x, x*x*x);

    return (MB * alpha) * 0.166666666;
}

vec4 calc_hg (float x)
{
    vec4 w = bezier_weights(x);

    vec2 g = vec2(w[0] + w[1], w[2] + w[3]);
    vec2 h = vec2(1.0 - w[1] / (w[0] + w[1]) + x,
                  1.0 + w[3] / (w[2] + w[3]) - x);

    return vec4(h,g);
}

vec4 bicubic_fetch(sampler2D texsource, vec2 texcoord, vec2 texsize_inv)
{
    /* return texture(texsource, texcoord); */

    vec2 coord_hg = fract(texcoord / texsize_inv - vec2(0.5));//texsize_inv);

    vec3 hg_x = calc_hg(coord_hg.x).xyz;
    vec3 hg_y = calc_hg(coord_hg.y).xyz;

    vec2 e_x = vec2(texsize_inv.x, 0);
    vec2 e_y = vec2(0, texsize_inv.y);

    vec2 coord10 = texcoord + hg_x.x * e_x;
    vec2 coord00 = texcoord - hg_x.y * e_x;
    
    vec2 coord11 = coord10 + hg_y.x * e_y;
    vec2 coord01 = coord00 + hg_y.x * e_y;

    coord10 -= hg_y.y * e_y;
    coord00 -= hg_y.y * e_y;

    vec4 texel00 = texture(texsource, coord00);
    vec4 texel01 = texture(texsource, coord01);
    vec4 texel10 = texture(texsource, coord10);
    vec4 texel11 = texture(texsource, coord11);

    vec4 mix0 = mix(texel00, texel01, hg_y.z);
    vec4 mix1 = mix(texel10, texel11, hg_y.z);

    return mix(mix0, mix1, hg_x.z);
}

vec4 interpolate_dof(vec3 small, vec3 med, vec3 large, float t)
{
    vec4 weights = clamp(t * dof_lerp_scale + dof_lerp_bias, 0, 1);
    weights.yz = min(weights.yz, vec2(1) - weights.xy);

    vec3 color = weights.y * small + weights.z * med + weights.w * large;
    
    float alpha = dot(weights.yzw, vec3(16.0/17.0, 1, 1));

    return vec4(color, alpha);
}

vec4 gather_samples(vec2 tex_coord)
{
    vec4 focused = texture(rgbz_buffer, tex_coord);

    vec4 small = get_small_blur(tex_coord);
    vec4 med = bicubic_fetch(shrunk_buffer, tex_coord, shrunk_buffer_size_inv);
    //vec4 med = texture(shrunk_buffer, tex_coord);
    //vec4 large = texture(blurred_buffer, tex_coord);
    vec4 large = bicubic_fetch(blurred_buffer, tex_coord, shrunk_buffer_size_inv);

    float near_coc = med.a;
    float depth = small.a;

    float far_coc = clamp(dof_world.z * depth + dof_world.w, 0, 1);

    float coc = max(near_coc, far_coc * far_coc_radius);

    vec4 dof = interpolate_dof(small.rgb, med.rgb, large.rgb, coc);

    return vec4((1-dof.a) * focused.rgb + dof.rgb,1);
}

void main (void)
{

    vec2 buff_size = 1/rgbz_buffer_size_inv;
    float pixelsize = max(buff_size.x, buff_size.y);
    buff_size /= pixelsize;

    float d = length(screen_pos * buff_size);
    float R = barrel_distortion;
    vec2 pos = (screen_pos * (R+d)) / (R+length(buff_size));

    vec2 tex_coord = pos * 0.5 + vec2(0.5);
    vec4 color = gather_samples(tex_coord);
    vec4 particle_value = texture(particle_overlay, tex_coord);

    frag_color = tone_map(particle_value + color * (1-particle_value.a));
}
