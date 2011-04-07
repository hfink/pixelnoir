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

#ifndef __CB_MATERIAL_PROCESSOR_H
#define __CB_MATERIAL_PROCESSOR_H

#include "cbcommon.h"
#include "Processor.h"

#include "rtr_format.pb.h"

#include "EffectProcessor.h"

namespace ColladaBakery {

    class Baker;

    /**
     * The material system we use in our simplified RTR format data model, 
     * does not keep the instantiation between effects and materials, except
     * that the representation of COLLADA materials instantiating the same
     * COLLADA effect are likely to use the same shader instance in our
     * runtime system. Parameters will be duplicated, though.
     */
    class MaterialProcessor : public Processor {
    public:

        struct BakeCache {
            string rtr_material_id;
            EffectProcessor::LayerToTextureMapInfoMap layer_to_map_info;
        };

        MaterialProcessor(Baker* baker);

        bool process(const CF::Object* cObject);
        bool post_process();

    private:

        string _c_id;
        CF::UniqueId _c_mat_id;
        CF::UniqueId _c_instantiated_effect_id;

    };

    typedef boost::shared_ptr<MaterialProcessor> MaterialProcessorRef;
}

#endif //__CB_MATERIAL_PROCESSOR_H