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

#include "Geometry.h"

Geometry::Geometry(const string& id, 
                   const GPUMeshRef& mesh, 
                   const TransformNodeRef& node,
                   const MaterialInstanceRef& material,
                   const string& mat_str_id) : 
    SceneObject(id, node),
    _mesh(mesh), _material_instance(material),
    _material_str_id(mat_str_id)
{
    update_bounding_volume();
}

void Geometry::prepare(const Shader& shader) const {
}

void Geometry::draw(const Shader& shader) const {
    list<GPUMeshRef>::const_iterator it;
    _mesh->draw(shader);
}

const BoundingVolume& Geometry::bounding_volume() const {

    //we only update the bounding volume, if the trafo has
    //changed
    if (SceneObject::transform_node()->has_changed()) {
        update_bounding_volume();
    }

    return _bounding_volume;
}

void Geometry::update_bounding_volume() const {
    const mat4& model_m = SceneObject::get_local_to_world();
    //TODO: we should use OBB in here... 

    //at the moment we use bounding spheres only

    //Note that the (non-uniform) scale operation would invalidate
    //the bounding sphere as well, at the moment
    const Sphere& sphere = _mesh->bounding_volume().sphere();
    vec4 center = vec4(sphere.center(), 1);
    vec4 ctr_m = model_m * center;
    
    //Note: this works only with uniform scale operations!

    //Transform a point on sphere to get the new radius
    vec4 p = center;
    p.z += sphere.radius();

    vec4 p_m = model_m * p;

    vec4 diff = (p_m - ctr_m);

    float new_radius = glm::length(diff);

    Sphere s_m(new_radius, vec3(ctr_m));

    _bounding_volume = BoundingVolume(s_m);
}
