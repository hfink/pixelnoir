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

#ifndef __CB_LIGHT_PROCESSOR_H
#define __CB_LIGHT_PROCESSOR_H

#include "cbcommon.h"
#include "Processor.h"
#include "COLLADAFWTypes.h"
#include "rtr_format.pb.h"

namespace ColladaBakery {
    class Baker;

    class LightProcessor : public Processor {

    public:
        typedef rtr_format::Light BakeCache;
        typedef map<string, string> NameMap;

        LightProcessor(Baker* baker);

        /**
         * There is a nasty bug in the current OpenCOLLADA Max exporter: 
         * If light source is a POINT light, the fea/near attenuation extra
         * strings are screwed up. However, the information we need to extract
         * is written into different extra tags. This workaround mapping
         * is stored in kPLightWorkaround until that bug is fixed in the
         * current exporter.
         */

        static const NameMap& kPLightWorkaround() {
            static const NameMap m = initialize_pworkaround();
            return m;
        }

        virtual bool process(const CF::Object* cObject);

        //The post processor will connect additional MAX extra data with
        //our light model
        virtual bool post_process();

    private:

        static NameMap initialize_pworkaround() {
            NameMap m;
            m["attenuation_near_start"] = "hotspot_beam";
            m["attenuation_near_end"] = "falloff";
            m["attenuation_far_start"] = "aspect_ratio";
            m["attenuation_far_end"] = "attenuation_near_start";
            return m;
        }

        //helper method
        float get_extra_float_content(const string& key);

        string _c_id;
        CF::UniqueId _c_unique_id;

        rtr_format::Light _rtr_light;

    };

    typedef boost::shared_ptr<LightProcessor> LightProcessorRef;
}

#endif //__CB_LIGHT_PROCESSOR_H