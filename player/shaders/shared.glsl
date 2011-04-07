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


#define MAX_LIGHT_COUNT 12

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2
#define SHADOWED_SPOT_LIGHT 3

uniform Shared
{
    vec3 ambient;
    int  light_types[MAX_LIGHT_COUNT];
    int light_shadow_ids[MAX_LIGHT_COUNT];
    vec3 light_positions[MAX_LIGHT_COUNT];
    vec3 light_directions[MAX_LIGHT_COUNT];
    vec3 light_intensities[MAX_LIGHT_COUNT];
    vec4 light_attenuations[MAX_LIGHT_COUNT];
    vec2 light_spot_attenuations[MAX_LIGHT_COUNT];
    
    vec4 shadow_matrices[MAX_LIGHT_COUNT*4];
    float shadowmap_min_variance;
    float shadow_bleed_bias;
    
    int light_count;
    vec3 camera_world_position;
};

