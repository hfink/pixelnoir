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

#include "MaterialProcessor.h"
#include "Baker.h"
#include "COLLADAFWMaterial.h"

using namespace ColladaBakery;

MaterialProcessor::MaterialProcessor(Baker* baker) : 
Processor(baker)
{
}

bool MaterialProcessor::process(const CF::Object* cObject) {
    const CF::Material * c_mat = static_cast<const CF::Material*>(cObject);

    _c_mat_id = c_mat->getUniqueId();

    //At the moment there is no obvious way to access param-overrides of a 
    //material with OpenCOLLADA. TODO: double-check this!

    //We need post-processing as we must resolve the instantiated effect
    //The post-process stage of the effect processor has to run first.
    _baker->register_for_postprocess(shared_from_this(), 4);

    _c_id = _baker->get_id(c_mat);

    _c_instantiated_effect_id = c_mat->getInstantiatedEffect();

    return true;
}

bool MaterialProcessor::post_process() {

    //Process the actual effects
    
    //Check if this mataerial was actually used. Sometimes we find
    //zombie-materials in a COLLADA file. We want to exclude these from 
    //importing.
    if (_baker->cache().used_materials.count(_c_mat_id) == 0) {
        //Nothing will be baked or written.
        return true;
    }

    //Look up the effect cache
    BakerCache::EffectBakeCache::const_iterator it = 
                       _baker->cache().effects.find(_c_instantiated_effect_id);

    if (it != _baker->cache().effects.end()) {

        //add all used images to global used images array
        //(this enables the imageprocessor post-process stage to add
        //images to copy-queue of the bakery.)
        std::list<CF::UniqueId>::const_iterator it_used_img;
        for (it_used_img = it->second.used_images.begin();
             it_used_img != it->second.used_images.end();
             ++it_used_img)
        {
            _baker->cache().used_images.insert(*it_used_img);
        }
            
        //copy from prototype and bake
        rtr_format::Material rtr_material;
        rtr_material.CopyFrom(it->second.material_prototype);

        //set the id
        rtr_material.set_id(_c_id);

        //finally insert the material into the material cache
        BakeCache c;
        c.rtr_material_id = rtr_material.id();
        //we also need to pass the uv coord mapping
        c.layer_to_map_info = it->second.layer_to_map_info;

        BakerCache::MaterialBakeCache::value_type v(_c_mat_id, c);
        if (!_baker->cache().materials.insert(v).second) {
            cout << "Could not insert into material cache." << endl;
            return false;
        }

        //Finally, bake the material to DB
        bool b = _baker->write_baked(rtr_material.id(), &rtr_material);
        if (!b) {
            cout << "Baking of material " << rtr_material.id() << " failed." 
                 << endl;
            return false;
        }

        cout << endl;
        cout << "Wrote material: '" << rtr_material.id() << "'." << endl;
        cout << endl;

    } else {
        cout << "Could not resolve instantiated effect " << _c_instantiated_effect_id.toAscii()
             << "." << endl;
    }

    return true;
}