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

#include "AnimationBindingProcessor.h"

#include "COLLADAFWAnimationList.h"

#include "Baker.h"

#include "Utils.h"

using namespace ColladaBakery;

AnimationBindingProcessor::AnimationBindingProcessor(Baker* baker) :
    Processor(baker)
{}

bool AnimationBindingProcessor::process(const CF::Object* c_obj) {

    const CF::AnimationList* c_anim_l = 
                                   static_cast<const CF::AnimationList*>(c_obj);

    //cache the entries of this animation list
    _c_id = c_anim_l->getUniqueId();
    for (size_t i = 0; i<c_anim_l->getAnimationBindings().getCount(); ++i) {
        _animation_bindings.push_back(c_anim_l->getAnimationBindings()[i]);
    }

    //Note: the weight to insert must be smaller than the Animations postprocess
    //registration, as the animation post processor must run after the post
    //processors of this class.
    _baker->register_for_postprocess(shared_from_this(), -1);

    return true;
}

bool AnimationBindingProcessor::post_process() {

    //we can expect that all animation binding  requests have been set at this 
    //point, we will now write out those animation lists, that were actually 
    //requested

    //get the requests with our ID
    size_t num_requests = _baker->cache().animation_binding_requests.count(_c_id);

    BakerCache::AnimListToRTRTargetMap::iterator it = 
                         _baker->cache().animation_binding_requests.find(_c_id);

    while (num_requests > 0) {
        
        AnimationBindingList::const_iterator it_binding;

        for ( it_binding = _animation_bindings.begin(); 
              it_binding != _animation_bindings.end();
              ++it_binding )
        {
            BakerCache::RTRBinding rtr_binding;
            rtr_binding.c_anim_list_id = _c_id;
            rtr_binding.rtr_id = it->second;
            rtr_binding.c_anim_binding = *it_binding;
            BakerCache::AnimToRTRBindingMap::value_type v(it_binding->animation, 
                                                          rtr_binding);
            _baker->cache().animation_resolved_bindings.insert(v);
        }

        ++it;
        num_requests--;
    }

    return true;
}