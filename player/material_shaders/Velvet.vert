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

in vec2 uv_diffuse;
in vec2 uv_specular;
in vec2 uv_normalmap;
in vec2 uv_lightmap;
in vec3 normal;
in vec4 tangent;

out MaterialVaryings
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

void eval_material(void)
{
    mvaryings.position = (model * vertex).xyz;
    mvaryings.normal = normal_matrix * normal;
    mvaryings.tangent = normal_matrix * tangent.xyz;

    /* // Catch undefined tangents */
    /* if (any(isnan(mvaryings.tangent))) { */
    /*     mvaryings.tangent = cross(mvaryings.normal, vec3(0,1,0)); */
    /* } */

    mvaryings.bitangent =  tangent.w * cross( mvaryings.tangent,
                                              mvaryings.normal );
    
    mvaryings.uv_diffuse = uv_diffuse;
    mvaryings.uv_specular = uv_specular;
    mvaryings.uv_normalmap = uv_normalmap;
    mvaryings.uv_lightmap = uv_lightmap;
}
