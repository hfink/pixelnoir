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

#include "FreeLookController.h"
#include <math.h>
#include "utility.h"
#include "RtrPlayerConfig.h"

FreeLookController::FreeLookController(const CameraRef& cam)
{
    reset(cam);
}

void FreeLookController::reset(const CameraRef& cam)
{
    _cam = cam;

    _pos = cam->get_world_location();

    const mat4& view = cam->get_world_to_local();

    _yaw = atan2f(view[1][0],view[0][0]);
    _pitch = atan2f(view[2][1], view[2][2]);

    _yaw *= 180 / float(M_PI);
    _pitch *= 180 / float(M_PI);
}

void FreeLookController::move(int dx, int dy, 
                              const ivec3& strafe, float timediff) 
{
    float angle_factor = -config.freelook_angle_speed();
    float strafe_factor = config.freelook_strafe_speed() * timediff;

    mat4 matrix = glm::translate(_pos);

    _yaw  += dx * angle_factor;
    _pitch = rtr::min(170.f, rtr::max(10.f, _pitch + dy * angle_factor));

    matrix = matrix * glm::rotate(_yaw, WORLD_UP());
    matrix = matrix * glm::rotate(_pitch, WORLD_SIDE());

    vec3 translation_vec = vec3(strafe) * strafe_factor;
    matrix = matrix * glm::translate(translation_vec);

    _cam->override_transform(matrix); 

    vec4 pos = matrix * vec4(0,0,0,1);
    _pos = vec3(pos.x,pos.y, pos.z) / pos.w;
}
