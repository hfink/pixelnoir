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

#ifndef __GEOMETRY_H
#define __GEOMETRY_H

#include "SceneObject.h"
#include "Mesh.h"
#include "common.h"
#include "BoundingVolume.h"

#include "MaterialManager.h"

/**
 * A simple runtime-class representing an instance of a
 * GPU Mesh with a transform inherited from SceneObject.
 * Additional material information will be added. At the moment 
 * this class is mostly a stub.
 */
class Geometry : public SceneObject {

public:

    Geometry(const string& id,
             const GPUMeshRef& mesh,
             const TransformNodeRef& node,
             const MaterialInstanceRef& material,
             const string& mat_str_id);

    virtual ~Geometry() {}

    void draw(const Shader& shader) const;
    void prepare(const Shader& shader) const;

    const BoundingVolume& bounding_volume() const;

    int material_id() const { return _material_instance->material_id(); }

    const string& material_string_id() const { return _material_str_id; }

    const MaterialInstanceRef& material_instance() const 
    { 
        return _material_instance; 
    }

    void set_material_instance(const MaterialInstanceRef& mat) { _material_instance = mat; }

    void reset_material() { _material_instance.reset(); } 

private:
    //In future this will also be a collection of tuple<GPUMeshRef, Material>
    //However, we have no real material system yet.
    GPUMeshRef _mesh;
    MaterialInstanceRef _material_instance;

    string _material_str_id;

    void update_bounding_volume() const;

    mutable BoundingVolume _bounding_volume;
};

typedef shared_ptr<Geometry> GeometryRef;

#endif //__GEOMETRY_H
