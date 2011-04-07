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
    vec3 normal;
    vec3 position;
} mvaryings;

uniform Material
{
    float sigma;
    float shininess;
    vec3 specular;
    vec4 color;
};

uniform sampler2D tex;

vec3 calc_illumination(vec3 albedo, float shininess)
{
    const float PI = 3.14159265359;

    float sigma2 = sigma*sigma;
    float A = 1-sigma2 / (2 * (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    vec3 normal = normalize(mvaryings.normal);
    vec3 view = normalize(camera_world_position - mvaryings.position);
    vec3 total_intensity = vec3(0,0,0);

    float eta_o = acos(dot(normal,view));
    vec3 Vp = normalize(view - dot(normal,view) * normal);

    for (int i = 0; i < light_count; ++i) {
        vec3 light;
        vec3 intensity;

        eval_light(i, mvaryings.position, light, intensity);

        float eta_i = acos(dot(normal,light));
        vec3 Lp = normalize(light - dot(normal,light) * normal);

        float alpha = max(eta_o, eta_i);
        float beta = min(eta_o, eta_i);

        vec3 half_vector = normalize(view+light);
        float cosine = max(0,dot(normal,light));
        float diff = A + B * max(0,dot(Lp,Vp)) * sin(alpha) * tan(beta);
        float spec = pow(max(0,dot(half_vector,normal)), shininess);


        vec3 l = intensity * cosine;

        total_intensity +=  l * (albedo * diff + specular * spec);
    }
    
    return total_intensity;
}

vec4 eval_material(void)
{
    float v = texture(tex, mvaryings.tex_coord).x;
    vec3 albedo = color.xyz;
    return vec4(calc_illumination(albedo, 10+v*50),1);
}
