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

#ifndef __FREE_LOOK_CONTROLLER_H
#define __FREE_LOOK_CONTROLLER_H

#include "Camera.h"

/**
 * A very rudimentary Free-Look Camera behaviour.
 */
class FreeLookController {

public:
    /**
     * Creates a new free look camera controller. 
     * @param cam The camera to control.
     * @param initial_matrix The initial transformation of the camera.
     */
    FreeLookController(const CameraRef& cam);

    void reset(const CameraRef& cam);

    /**
     * Perform the free-look move.
     * @param dx The change in X direction in pixels.
     * @param dy The change in Y direction in pixels.
     * @param strafe Strafing direction.
     */
    void move(int dx, int dy, const ivec3& strafe, float timediff);

    const CameraRef& cam() const { return _cam; }

private:

    vec3 WORLD_UP() { return vec3(0,0,1); }
    vec3 WORLD_SIDE() { return vec3(1,0,0); }
    
    float _yaw;
    float _pitch;
    vec3 _pos;
    CameraRef _cam;
};

#endif //__FREE_LOOK_CONTROLLER_H
