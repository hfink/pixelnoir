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

#ifndef __LIGHT_H
#define __LIGHT_H

#include "SceneObject.h"
#include "AnimEvaluator.h"
#include "Transform.h"
#include "common.h"

class Light : public SceneObject {

public:

    typedef enum {
        POINT,
        DIRECTIONAL,
        SPOT
    } Type;

    Light(const string& id,
          const Type& type,
          const vec3& intensity,
          bool use_shadowmaps,
          float multiplier,
          AnimEvaluator& evaluator,
          const TransformNodeRef& node);

    virtual ~Light() {}

    vec3 get_intensity();
    void set_intensity(const vec3& intensity);

    float get_multiplier();
    void set_multiplier(float multiplier);

    bool use_shadowmaps() const { return _use_shadowmaps; }

    vec3 calc_multiplied_intensity();

    Type get_type() { return _type; }

    vec3 world_position();
    vec3 world_direction();

    vec4 attenuation() { return _attenuation; }
    vec2 spot_attenuation() { return _spot_attenuation; }
    int shadowmap_id() { return _shadowmap_id; }

    void set_attenuation(vec4 attenuation) { _attenuation = attenuation; }
    void set_spot_attenuation(vec2 spot_attenuation) { 
        _spot_attenuation = spot_attenuation; 
    }
    void set_shadowmap_id(int shadowmap_id) { _shadowmap_id = shadowmap_id; }
private:
    Type _type;
    AnimListener<vec4> _intensity;
    AnimListener<float> _multiplier;
    
    // This stores constant, linear and quadratic attenuation in a vec3
    // Only needed for point-, and spotlights.
    vec4 _attenuation; 

    // Spot angle and exponent. Only needed for spotlights.
    vec2 _spot_attenuation;

    int _shadowmap_id;

    bool _use_shadowmaps;
};

typedef shared_ptr<Light> LightRef;

#endif //__LIGHT_H
