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

#include "LightProcessor.h"
#include "COLLADAFWLight.h"

#include <sstream>

#include "common_const.h"

#include "Utils.h"
#include "Baker.h"
#include "BakerCache.h"

using namespace ColladaBakery;

LightProcessor::LightProcessor(Baker* baker) :
    Processor(baker)
{}

bool LightProcessor::process(const CF::Object* cObject) {
    
    const CF::Light * c_light = static_cast<const CF::Light *>(cObject);

    //We just take what we need from COLLADA's light mode. The rest is filled
    //with data from 3dsmax extra data (OpenCOLLADA profile).

    _c_id = _baker->get_id(c_light);
    _c_unique_id = c_light->getUniqueId();

    _rtr_light.set_id(_c_id);

    //common properties
    const CF::Color& c_clr = c_light->getColor();

    //Catch animated colors
    if (c_clr.getAnimationList().isValid()) {
        string rtr_id = _c_id + rtr::Targets::LIGHT_INTENSITY_TARGET();
        BakerCache::AnimListToRTRTargetMap::value_type v(c_clr.getAnimationList(),
                                                         rtr_id);
        _baker->cache().animation_binding_requests.insert(v);
        _baker->cache().animation_used_animlists.push_back(
            c_clr.getAnimationList());
    }

    _rtr_light.mutable_intensity()->set_x(static_cast<float>(c_clr.getRed()));
    _rtr_light.mutable_intensity()->set_y(static_cast<float>(c_clr.getGreen()));
    _rtr_light.mutable_intensity()->set_z(static_cast<float>(c_clr.getBlue()));

    //Note: we ignore COLLADA's attenuation model for now. Instead we will
    //make use of the exported 3dsmax OpenCOLLADA extra tags (see post
    //process stage).

    //set actual types
    if (c_light->getLightType() == CF::Light::POINT_LIGHT) {
        _rtr_light.set_type(rtr_format::Light::POINT);
    } else if (c_light->getLightType() == CF::Light::SPOT_LIGHT) {
        _rtr_light.set_type(rtr_format::Light::SPOT);
    } else if (c_light->getLightType() == CF::Light::DIRECTIONAL_LIGHT) {
        _rtr_light.set_type(rtr_format::Light::DIRECTIONAL);
    } else if (c_light->getLightType() == CF::Light::AMBIENT_LIGHT) {
        cout << "Warning: Lights of type 'ambient' are not supported. "
             << "Skipping import." << endl << endl;
        return true;
    } else {
        cerr << "Unknown light type" << endl;
        return false;
    }

    //finally add to post process stage to process extra tags
    _baker->register_for_postprocess(shared_from_this());

    return true;
}

bool LightProcessor::post_process() {

    const string light_tag = "max_light";
    string near_att_start_key = "attenuation_near_start";
    string far_att_start_key = "attenuation_far_start";
    string near_att_end_key = "attenuation_near_end";
    string far_att_end_key = "attenuation_far_end";

    //Workaround for OpenCOLLADA bug (see LightProcessor.h)
    if (_rtr_light.type() == rtr_format::Light::POINT) {
        near_att_start_key = kPLightWorkaround().at(near_att_start_key);
        far_att_start_key = kPLightWorkaround().at(far_att_start_key);
        near_att_end_key = kPLightWorkaround().at(near_att_end_key);
        far_att_end_key = kPLightWorkaround().at(far_att_end_key);
    }

    //Get near attenuation and set, if used
    string use_near_att = _baker->extra_handler().get_content(
        _c_unique_id,
        ExtraDataHandler::k3DSMaxProfile(),
        light_tag+"/use_near_attenuation");

    if (use_near_att == "1") {
        float v = get_extra_float_content(light_tag + "/" + near_att_start_key);
        _rtr_light.mutable_near_attenuation()->set_start(v);
        v = get_extra_float_content(light_tag+"/" + near_att_end_key);
        _rtr_light.mutable_near_attenuation()->set_end(v);
    }

    //Get far attenuation and set, if used
    string use_far_att = _baker->extra_handler().get_content(
        _c_unique_id,
        ExtraDataHandler::k3DSMaxProfile(),
        light_tag+"/use_far_attenuation");

    if (use_far_att == "1") {
        float v = get_extra_float_content(light_tag+"/" + far_att_start_key);
        _rtr_light.mutable_far_attenuation()->set_start(v);
        v = get_extra_float_content(light_tag+"/" + far_att_end_key);
        _rtr_light.mutable_far_attenuation()->set_end(v);
    }

    if (_rtr_light.type() == rtr_format::Light::SPOT) {
        //get the falloff and hotspot angles
        float v = get_extra_float_content(light_tag+"/hotspot_beam");
        _rtr_light.mutable_spot_attenuation()->set_hotspot_angle(v);
        v = get_extra_float_content(light_tag+"/falloff");
        _rtr_light.mutable_spot_attenuation()->set_falloff_angle(v);
    }

    //get the shadow map option. We issue a warning, is shadow maps were
    //requested, but no near/far attenuation was set...
    string shadow_type = _baker->extra_handler().get_content(
        _c_unique_id,
        ExtraDataHandler::k3DSMaxProfile(),
        "shadow_attributes/type");
    
    _rtr_light.set_use_shadowmaps(false);

    if (shadow_type == "type_map") {

        //We require both attenuation to be set in order to actiate shadow maps
        if (! ( _rtr_light.has_near_attenuation() && 
                _rtr_light.has_far_attenuation() ) )
        {
            cout << "Warning: Light '" << _rtr_light.id() << "' requested "
                 << "dynamic shadow maps, but no near/far attenuation was "
                 << "properly set. Deactivating shadow maps for this light "
                 << "source." << endl;
        } else {
            _rtr_light.set_use_shadowmaps(true);
        }
    }

    //Finally, we extract the "multiplier" value
    float v = get_extra_float_content(light_tag+"/multiplier");
    _rtr_light.set_multiplier(v);

    BakerCache::LightBakeCache::value_type entry( _c_unique_id,
                                                  _rtr_light );

    bool b = _baker->cache().lights.insert(entry).second;
    assert(b);

    return true;
}

float LightProcessor::get_extra_float_content(const string& key) {
    
    float value = 0;
    //Get attenuation values
    string str_value = _baker->extra_handler().get_content(
        _c_unique_id,
        ExtraDataHandler::k3DSMaxProfile(),
        key);

    if (!str_value.empty()) {
        
        std::stringstream ss(str_value);
        if ((ss >> value).fail()) {
            cout << "Could not convert string '" << str_value
                 << " to float value. Using 0 instead." << endl;
            value = 0;
        }

    } else {
        cout << "Error: Could not retrieve "
             << key
             << ".";
    }

    return value;
}