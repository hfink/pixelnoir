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
    vec2 tex_coord;
    vec2 tex_coord2;
    vec3 normal;
    vec3 position;
} mvaryings;

uniform Material
{
    float shininess;
    vec3 specular_color;
};

uniform sampler2D light_map;
uniform sampler2D albedo_tex;

vec3 calc_illumination(vec3 albedo)
{
    vec3 normal = normalize(mvaryings.normal);
    vec3 view = normalize(camera_world_position - mvaryings.position);

    vec3 total_intensity = vec3(0);

    for (int i = 0; i < light_count; ++i) {
        vec3 light;
        vec3 intensity;

        eval_light(i, mvaryings.position, light, intensity);
                               
        float cosine = max(0,dot(normal, light));
        float spec = pow(max(0, dot(half_vector, normal)), shininess);

        vec l = intensity * cosine;

        total_intensity += l * (albedo + specular_color * spec);
    }

    return total_intensity;        
}

vec4 eval_material(void)
{
    vec3 albedo = texture(albedo_tex, mvaryings.tex_coord).xyz;
    vec3 ambient = texture(light_map, mvaryings.tex_coord2).xyz;

    return vec4((calc_illumination(albedo) + ambient*albedo, 1);
}
    
