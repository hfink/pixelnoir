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

#ifndef OBJECTINDEX_H
#define OBJECTINDEX_H

#include "common.h"
#include "rtr_format.pb.h"

// This class encapsulates accessing protocol-buffer objects by name
// This is a placeholder for Tokyo cabinet file access.
class ObjectIndex
{
    map<string, shared_ptr<rtr_format::Material> > _material_map;
    map<string, shared_ptr<rtr_format::Mesh> > _mesh_map;
    map<string, shared_ptr<rtr_format::LayerSource> > _layer_source_map;
    map<string, shared_ptr<rtr_format::Animation> > _animation_map;

    public:

    void insert_material(shared_ptr<rtr_format::Material> material);
    void insert_mesh(shared_ptr<rtr_format::Mesh> mesh);
    void insert_layer_source(shared_ptr<rtr_format::LayerSource> source);
    void insert_animation(shared_ptr<rtr_format::Animation> animation);

    shared_ptr<rtr_format::Material> get_material(const string& id) const;
    shared_ptr<rtr_format::Mesh> get_mesh(const string& id) const;
    shared_ptr<rtr_format::LayerSource> get_layer_source(const string& id) const;
    shared_ptr<rtr_format::Animation> get_animation(const string& id) const;
    shared_ptr<rtr_format::Geometry> get_geometry(const string& id) const;
    shared_ptr<rtr_format::TransformNode> get_transform_node(const string& id) const;
    shared_ptr<rtr_format::Light> get_light(const string& id) const;
};

#endif
