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


uniform sampler2DArray shadowmaps;
uniform float min_variance;

float linstep(float minv, float maxv, float v)
{
    return maxv == minv ? 1.0 : clamp((v - minv) / (maxv - minv), 0.0, 1.0);
}

float reduce_lightbleeding(float p_max)
{
    return linstep(shadow_bleed_bias, 1, p_max);
}

float chebyshev_upper_bound(vec2 moments, float t)
{
    if (any(isnan(moments))) return 1.0;

    float p = step(t,moments.x);
    float variance = moments.y - (moments.x*moments.x);

    variance = max(variance, shadowmap_min_variance);
    
    float d = t - moments.x;
    float p_max = variance / (variance + d*d);

    return max(p,reduce_lightbleeding(p_max));    
}

float attenuate(vec4 attenuation, float d)
{
    float att = linstep(attenuation.x, attenuation.y, d);
    att *= linstep(attenuation.w, attenuation.z, d);
    return att;
}
 
void eval_spotlight(in int index, in vec3 position,
                    out vec3 intensity, out vec3 direction)
{
    vec3 light_pos = light_positions[index];

    direction = light_pos - position;
    float d = length(direction);
    direction = normalize(direction);

    float cos_angle = dot(-direction, light_directions[index]);

    float cos_min_angle = light_spot_attenuations[index].x;
    float cos_max_angle = light_spot_attenuations[index].y;

    float att = attenuate(light_attenuations[index], d);
    att *= linstep(cos_max_angle, cos_min_angle, cos_angle);

    intensity = light_intensities[index] * att;        
}

void eval_light(in int index, in vec3 position, 
                out vec3 light_vector, out vec3 intensity)
{
    int type = light_types[index];

    if (type == POINT_LIGHT) {
        vec3 light_pos = light_positions[index];

        light_vector = light_pos - position;
        
        float d = length(light_vector);
        light_vector = normalize(light_vector);

        float att = attenuate(light_attenuations[index], d);

        intensity = light_intensities[index] * att;
    } else if (type == DIRECTIONAL_LIGHT) {
        light_vector = -light_directions[index];
        intensity = light_intensities[index];
    } else if (type == SPOT_LIGHT) {
        eval_spotlight(index, position, intensity, light_vector);
    }  else /* if (type == SHADOWED_SPOT_LIGHT) */ {
        eval_spotlight(index, position, intensity, light_vector);

        int shadow_id = light_shadow_ids[index];

        //This is a workaround for Catalyst 11.1, where uploading mat4
        //arrays had a bug. Therefeore, until this bug is fixed, we use
        //four vec4's instead of one mat4.

        mat4 shadow_mat = mat4( shadow_matrices[shadow_id*4], 
                                shadow_matrices[shadow_id*4+1], 
                                shadow_matrices[shadow_id*4+2], 
                                shadow_matrices[shadow_id*4+3] );

        vec4 shadowmap_pos =  shadow_mat * vec4(position,1);

        float local_depth = shadowmap_pos.w;
        vec2 shadow_coord = shadowmap_pos.xy/shadowmap_pos.w;
        vec2 moments = texture(shadowmaps, 
                               vec3(shadow_coord, 
                                    shadow_id)).xy;

        float shadow = chebyshev_upper_bound(moments, local_depth);

        intensity *= shadow;       
    } 
}
