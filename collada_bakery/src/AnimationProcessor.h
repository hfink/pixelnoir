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

#ifndef __CB_ANIMATION_PROCESSOR_H
#define __CB_ANIMATION_PROCESSOR_H

#include "cbcommon.h"
#include "Processor.h"
#include "COLLADAFWTypes.h"

#include "COLLADAFWAnimation.h"
#include "COLLADAFWAnimationCurve.h"

#include "rtr_format.pb.h"

namespace ColladaBakery {
    
    class Baker;

    class AnimationProcessor : public Processor {

    public:

        AnimationProcessor(Baker* baker);

        virtual bool process(const CF::Object* cObject);
        virtual bool post_process();
    
    private:

        rtr_format::Animation _rtr_anim;

        rtr_format::Animation_Sampler* _rtr_input_sampler;
        rtr_format::Animation_Sampler* _rtr_output_sampler;

        void transpose_mat_output();

        void insert_step_interpolation(const CF::AnimationCurve * c_anim_curve,
                                       vector<CF::AnimationCurve::InterpolationType>& interpolation_types);

        float calc_diff(const float* c_in, const float* c_out, int num_segment, size_t c_out_dimension);

        bool _output_is_transposed;

        CF::UniqueId _c_id;

        struct HardCutLogInfo {
            float time;
            float diff;
        };

        typedef list<HardCutLogInfo > HardCutLogInfoList;
        HardCutLogInfoList _hard_cut_insertions;

    };

    typedef boost::shared_ptr<AnimationProcessor> AnimationProcessorRef; 
}

#endif //__CB_ANIMATION_PROCESSOR_H