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

#include "CameraProcessor.h"
#include "COLLADAFWCamera.h"
#include "Utils.h"
#include "Baker.h"
#include "BakerCache.h"
#include "common_const.h"

using namespace ColladaBakery;

CameraProcessor::CameraProcessor(Baker* baker) :
    Processor(baker)
{}

bool CameraProcessor::process(const CF::Object* cObject) {

    const CF::Camera* c_cam = static_cast<const CF::Camera*>(cObject);

    string c_id = _baker->get_id(c_cam);

    //Since we might import animations of either a FOV X or a FOV Y, 
    //we have to keep track of the FOV axis. Our Camera is able to choose 
    //between the two.

    float c_y_fov = c_cam->getYFov();
    float c_x_fov = c_cam->getXFov();

    rtr_format::Camera rtr_cam;
    rtr_cam.set_id(c_id);

    //We also allow both FOV values to be animated for ZOOM effects
    CF::UniqueId c_anim_list;

    if ( (c_y_fov != 0) && (c_x_fov != 0) ) {
        cout << "Warning: Camera '" << c_cam->getOriginalId() << "' has Y FOV"
             << " and X FOV defined. We have no concept of aspect ratio in "
             << "here, we use Y FOV only." << endl;
        rtr_cam.set_fov_angle(c_y_fov);
        rtr_cam.set_fov_axis(rtr_format::Camera::Y_AXIS);
    } else if ( (c_y_fov == 0) && (c_x_fov == 0) ) {
        cout << "Warning: Camera '" << c_cam->getOriginalId() << "' has no" 
             << " field of view properly defined. Using default of FOV Y = 45."
             << endl;
        rtr_cam.set_fov_angle(45.0f);
        rtr_cam.set_fov_axis(rtr_format::Camera::Y_AXIS);
    } else if (c_y_fov != .0f) {
        rtr_cam.set_fov_angle(c_y_fov);
        rtr_cam.set_fov_axis(rtr_format::Camera::Y_AXIS);
        c_anim_list = c_cam->getYFov().getAnimationList();
    } else if (c_x_fov != .0f) {
        rtr_cam.set_fov_angle(c_x_fov);
        rtr_cam.set_fov_axis(rtr_format::Camera::X_AXIS);  
        c_anim_list = c_cam->getXFov().getAnimationList();
    }

    if (c_anim_list.isValid()) {
        string rtr_id = c_id + rtr::Targets::CAM_FOV_TARGET();
        BakerCache::AnimListToRTRTargetMap::value_type v(c_anim_list,
                                                         rtr_id);
        _baker->cache().animation_binding_requests.insert(v);
        _baker->cache().animation_used_animlists.push_back(
            c_anim_list);
    }

    rtr_cam.set_z_near(static_cast<float>(c_cam->getNearClippingPlane()));
    rtr_cam.set_z_far(static_cast<float>(c_cam->getFarClippingPlane()));

    //memorize for later resolution
    BakerCache::CameraBakeCache::value_type entry( c_cam->getUniqueId(),
                                                   rtr_cam );
    
    bool b = _baker->cache().cameras.insert(entry).second;
    
    assert(b);

    return true;
}