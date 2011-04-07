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
    vec2 uv_lightmap;
    vec3 normal;
    vec3 position;
    vec3 tangent;
    vec3 bitangent;
} mvaryings;

uniform Material
{
    float shininess;
    float velvet_factor;
};

uniform sampler2D diffuse_tex;
uniform sampler2D specular_tex;
uniform sampler2D lightmap_tex;
uniform sampler2D normal_tex;

float velvet_lobe(vec3 V, vec3 N)
{
    float cos_angle = max(0,dot(V,N));

    return pow(1-cos_angle, shininess) * velvet_factor;
}

vec3 calc_illumination(vec3 albedo, vec3 specular_color, vec3 indirect_ill)
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

    vec3 total_intensity = vec3(0);

    for (int i = 0; i < light_count; ++i) {
        vec3 light_ws;
        vec3 intensity;

        eval_light(i, mvaryings.position, light_ws, intensity);

        float cosine = max(0,dot(normal_ws, light_ws));

        float spec = velvet_lobe(view_ws, normal_ws);

        vec3 l = intensity * cosine;
        total_intensity += l * ((albedo) + (specular_color * spec));
        
    }

    total_intensity += indirect_ill * albedo;

    return total_intensity;        
}

vec4 eval_material(void)
{
    vec3 albedo = texture(diffuse_tex, mvaryings.uv_diffuse).xyz;
    vec3 specular = texture(specular_tex, mvaryings.uv_specular).xyz;
    vec3 indirect_ill = texture(lightmap_tex, mvaryings.uv_lightmap).xyz;

    vec3 ill = calc_illumination(albedo, specular, indirect_ill);

    return vec4(ill, 1);
}
