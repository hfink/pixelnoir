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

#ifndef __BOUNDING_VOLUME_H
#define __BOUNDING_VOLUME_H

#include "Camera.h"

/**
 * A bounding sphere with a center and a particular radius.
 */
class Sphere {
public:

    Sphere() : 
      _radius(0), _center(vec3(0)) {}

    Sphere(float radius, const vec3& center) : 
      _radius(radius), _center(center) {}

    float radius() const { return _radius; }
    const vec3& center() const { return _center; }
    static Sphere unite(const Sphere& a, const Sphere& b);
private:
    float _radius;
    vec3 _center;
};

/**
 * Axis-Aligned Box. AABB is represented by its center and the positive
 * half-diagonal.
 */
class AABB {

public:

    /**
     * Creates a new AAB based on its minimum and maximum coordinates.
     */
    AABB(const vec3& min, const vec3& max) : 
        _center((max + min)*0.5f),
        _half_diagonal((max - min)*0.5f) {}

    const vec3& center() const { return _center; }
    const vec3& half_diagonal() const { return _half_diagonal; }
    
    static AABB unite(const AABB& a, const AABB& b);
private:
    vec3 _center;
    vec3 _half_diagonal;

};

/** 
 * A bounding volume holds different BV representations, atm Sphere and AABB.
 */
class BoundingVolume {
public:
    BoundingVolume();
    //BoundingVolume(const AABB& aabb, const Sphere& sphere);
    BoundingVolume(const Sphere& sphere);
    //const AABB& aabb() const { return _aabb; }
    const Sphere& sphere() const { return _sphere; }
    
private:
    //AABB _aabb;
    Sphere _sphere;
};

typedef enum {
    OUTSIDE,
    INSIDE,
    INTERSECTING
} TestResult;

/**
 * Test the intersection of an AABB with a plane. Code based on 
 * Rrealtime rendering ed3 pg. 756.
 */
inline TestResult intersect_aabb_plane(const AABB& box, const vec4& plane) {

    float e = box.half_diagonal().x * glm::abs(plane.x) + 
              box.half_diagonal().y * glm::abs(plane.y) + 
              box.half_diagonal().z * glm::abs(plane.z);

    vec4 c(box.center(), 1);
    float s = glm::dot(c, plane); 
    
    if (s-e > 0)
        return OUTSIDE;
    
    if (s+e < 0)
        return INSIDE;

    return INTERSECTING;
}

/**
 * Test the intersection between AABB and a frustum. Code based on 
 * Tealtime rendering, Ed. 3, pg. 777
 */
inline TestResult intersect_aabb_frustum(const AABB& box, const Frustum& f) {

    bool intersecting = false;
    for ( int i = 0; i < 6; ++i ) {
        TestResult result = intersect_aabb_plane(box, f.get_plane(i));
        if (result == OUTSIDE)
            return OUTSIDE;
        else if (result == INTERSECTING)
            intersecting = true;
    }

    if (intersecting)
        return INTERSECTING;
    else
        return INSIDE;

}

#endif //__BOUNDING_VOLUME_H