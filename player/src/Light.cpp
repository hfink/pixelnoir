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

#include "Light.h"

#include "common_const.h"

Light::Light(const string& id,
             const Type& type,
             const vec3& intensity,
             bool use_shadowmaps,
             float multiplier,
             AnimEvaluator& evaluator,
             const TransformNodeRef& node):
    SceneObject(id, node),
    _type(type),
    _intensity(evaluator, id+rtr::Targets::LIGHT_INTENSITY_TARGET()),
    _multiplier(evaluator, id+rtr::Targets::LIGHT_MULTIPLIER_TARGET()),
    _use_shadowmaps(use_shadowmaps)
{
    set_intensity(intensity);
    set_multiplier(multiplier);
}

vec3 Light::get_intensity() {
    //Note: this is the easiest workaround to compensate between
    //4-component animated colors and the required 3-component vector
    //for our intensity. In future we might compensate this in the bakery
    //already
    return vec3(_intensity.value());
}

void Light::set_intensity(const vec3& intensity) {
    _intensity.set(vec4(intensity, 1));
}

float Light::get_multiplier() {
    return _multiplier.value();
}

void Light::set_multiplier(float multiplier) {
    _multiplier.set(multiplier);
}

vec3 Light::calc_multiplied_intensity() {
    return get_intensity() * get_multiplier();
}

// vec3 Light::get_world_space_representation() {
// 	vec3 result(0, 0, 0);
// 	if (_type == POINT) {
// 		vec4 location = get_local_to_world()*vec4(0, 0, 0, 1);
// 		result = vec3(location);
// 	} else if (_type == DIRECTIONAL) {
        
// 		//TODO: replace with cached matrices
// 		//by default a directional light is pointing down the 
// 		//Z-axis
// 		glm::mat3 normalMatrix = glm::mat3(glm::transpose(get_world_to_local()));
// 		result = normalMatrix*vec3(0, 0, 1);
// 	}
// 	return result;
// }

vec3 Light::world_position()
{
    vec4 location = get_local_to_world()*vec4(0, 0, 0, 1);
    return vec3(location);
}


vec3 Light::world_direction()
{
    return normalize(vec3(get_local_to_world() * vec4(0,0,-1,0)));
};
