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

#include "BoundingVolume.h"

Sphere Sphere::unite(const Sphere& a, const Sphere& b) {
    
    //First check if the bigger one contains the smaller one
    const Sphere * bigger = NULL;
    const Sphere * smaller = NULL;

    if (a.radius() > b.radius()) {
        bigger = &a;
        smaller = &b;
    } else {
        bigger = &b;
        smaller = &a;
    }

    vec3 center_diff = smaller->center() - bigger->center();

    float center_diff_length = glm::length(center_diff);
    float center_diff_length_inv = 1.0f/center_diff_length;

    //If we are inside the bigger sphere, just return the bigger sphere
    if ( (center_diff_length + smaller->radius()) <= bigger->radius())
        return *bigger;

    //In all other cases we will have to find a tangent bounding sphere

    float merged_radius = 
        0.5f * ( center_diff_length + 
                 smaller->radius() +
                 bigger->radius() );

    float factor = 0.5f * ( smaller->radius() + 
                            center_diff_length - 
                            bigger->radius() ) * center_diff_length_inv;

    vec3 merged_center = bigger->center() + factor * center_diff;

    return Sphere(merged_radius, merged_center);
}

AABB AABB::unite(const AABB& a, const AABB& b) {
 
    vec3 a_min = a.center() - a.half_diagonal();
    vec3 a_max = a.center() + a.half_diagonal();

    vec3 b_min = b.center() - b.half_diagonal();
    vec3 b_max = b.center() + b.half_diagonal();

    vec3 new_min( glm::min(a_min.x, b_min.x),
                  glm::min(a_min.y, b_min.y),
                  glm::min(a_min.z, b_min.z) );

    vec3 new_max( glm::max(a_max.x, b_max.x),
                  glm::max(a_max.y, b_max.y),
                  glm::max(a_max.z, b_max.z) );

    return AABB(new_min, new_max);
}

BoundingVolume::BoundingVolume() :
    _sphere(0, vec3(0))
{}

BoundingVolume::BoundingVolume(const Sphere& sphere) :
    _sphere(sphere)
{}
