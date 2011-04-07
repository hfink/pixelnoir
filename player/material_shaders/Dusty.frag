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

in MaterialVaryings
{
    vec2 uv_diffuse;
    vec2 uv_specular;
    vec2 uv_normalmap;
    vec2 uv_ambientmap;
    vec3 normal;
    vec3 position;
    vec3 tangent;
    vec3 bitangent;
} mvaryings;

uniform Material
{
    float shininess;
    float dust_thickness;
    float dust_min_angle;
    float dust_exponent;
    vec3 dust_color;
};

uniform sampler2D diffuse_tex;
uniform sampler2D specular_tex;
uniform sampler2D ambient_occ_tex;
uniform sampler2D normal_tex;
uniform sampler2D dust_noise_tex;

vec3 calc_illumination(vec3 albedo, vec3 specular_color, float ambient_occlusion)
{
    vec3 normal = normalize(mvaryings.normal);
    vec3 tangent = normalize(mvaryings.tangent);
    vec3 bitangent = normalize(mvaryings.bitangent);
    
    vec3 normal_ts = normalize(texture(normal_tex, mvaryings.uv_normalmap).xyz  - 0.5f);

    // Transform normalmap normal from tangent space to world space
    vec3 normal_ws = normalize(mat3( tangent, bitangent, normal) * normal_ts);

    vec3 view_ws = normalize(camera_world_position - mvaryings.position);

    //eventual parameter for "scaling" the bumpiness... 
    //normal_ts.z *= 0.6;
    //normal_ts = normalize(normal_ts);

    vec3 dust_intensity = vec3(0.0);
    vec3 total_intensity = vec3(0.0);

    for (int i = 0; i < light_count; ++i) {
        vec3 light_ws;
        vec3 intensity;

        eval_light(i, mvaryings.position, light_ws, intensity);

        vec3 half_vector_ws = normalize(view_ws+light_ws);
        float cosine = max(0,dot(normal_ws, light_ws));

        // Calculate specular factor
        // We have to do some tests to catch numeric problems :(
        float cos_HN = max(0, dot(half_vector_ws, normal_ws));
        float spec;
        spec = cos_HN == 0.0 ? 0.0 : pow(cos_HN, shininess);
        spec = shininess == 0.0 ? 0.0 : spec;

        vec3 l = intensity * cosine;
        total_intensity += l * ((albedo) + (specular_color * spec));

        dust_intensity += intensity * max(0, dot(normal_ws, light_ws))*dust_color;
    }

    //total_intensity *= ambient_occlusion;

    //dust_intensity += (dust_intensity + indirect_ill) * dust_color;

    float thickness = smoothstep(dust_min_angle, 1, max(0,normal_ws.z));

    vec2 dust_tex_coord = mvaryings.position.xy * 0.015;
    float noise_tex_factor = texture(dust_noise_tex, dust_tex_coord).r * 1.5;

    vec3 mixed_dust = mix(total_intensity, dust_intensity,
                          thickness * dust_thickness * noise_tex_factor) * ambient_occlusion;

    return mixed_dust;        
}

vec4 eval_material(void)
{
    vec3 albedo = texture(diffuse_tex, mvaryings.uv_diffuse).xyz;
    vec3 specular = texture(specular_tex, mvaryings.uv_specular).xyz;
    float ambient_occlusion = texture(ambient_occ_tex, mvaryings.uv_ambientmap).r;

    vec3 ill = calc_illumination(albedo, specular, ambient_occlusion);

    if (any(isnan(ill))) return vec4(1,0,0,1);

    return vec4(ill, 1);
}
