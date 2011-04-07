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

#ifndef __CB_EFFECT_PROCESSOR_H
#define __CB_EFFECT_PROCESSOR_H

#include "Processor.h"

#include "rtr_format.pb.h"

#include "COLLADAFWNode.h"
#include "COLLADAFWEffectCommon.h"

#include <limits>

namespace ColladaBakery {
    
    class Baker;

    /**
     * A COLLADA effect will create rtr_format::Material prototype object. 
     * The material processor's post process method will copy parameters 
     * and setting of this effect to its own rtr_format::Material
     * instance which is the one that is actually going to be written out.
     * That way the material processor still have the chance of overriding
     * certain parameters.
     *
     *
     * Note that we might not import effects/materials from COLLADA in very
     * conformant way. We rather just take the most important things we can 
     * get.
     * 
     * This might subject of change in future.
     */
    class EffectProcessor : public Processor {
    
    public:

        //We allow both, the OpenCOLLADA texture map id OR a string semantic
        //to be used for UV coord binding. We prefer, however, to use the
        //CF::TextureMapId
        static const CF::TextureMapId& kInvalidTextureMapId() {
            static const CF::TextureMapId v = 
                std::numeric_limits<CF::TextureMapId>::max();
            return v;
        }

        struct TextureMapInfo {
            CF::TextureMapId c_texture_map_id;
            string c_texture_map_string;
            TextureMapInfo() : c_texture_map_id(kInvalidTextureMapId()) {}
        };

        typedef map<string, TextureMapInfo> LayerToTextureMapInfoMap;

        //This is the cache that the material processor will be able to have
        //access to
        struct BakeCache {
            rtr_format::Material material_prototype;
            //a list of images as used by this effect
            std::list<CF::UniqueId> used_images;
            LayerToTextureMapInfoMap layer_to_map_info;
        };

        EffectProcessor(Baker* baker);
        bool process(const CF::Object* cObject);
        bool post_process();

    private:

        //helper method to find a named sampelr (as used by extra tags)
        const CF::Sampler * find_sampler(const string& id);

        string get_string_for_color(const COLLADAFW::Color& c_color);

        rtr_format::Material _rtr_material_prototype;
        CF::UniqueId _c_id;
        CF::EffectCommon _c_fx_cmn;
    };

    typedef boost::shared_ptr<EffectProcessor> EffectProcessorRef;
}

#endif //__CB_EFFECT_PROCESSOR_H
