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

#include "ObjectIndex.h"

shared_ptr<rtr_format::Material> ObjectIndex::get_material(const string& id) const
{
    return _material_map.at(id);
}

shared_ptr<rtr_format::Mesh> ObjectIndex::get_mesh(const string& id) const
{
    return _mesh_map.at(id);
}

shared_ptr<rtr_format::LayerSource> ObjectIndex::get_layer_source(const string& id) const
{
    return _layer_source_map.at(id);
}

shared_ptr<rtr_format::Animation> ObjectIndex::get_animation(const string& id) const
{
    return _animation_map.at(id);
}

shared_ptr<rtr_format::Geometry> ObjectIndex::get_geometry(const string& id) const
{
    return shared_ptr<rtr_format::Geometry>();
}

shared_ptr<rtr_format::TransformNode> ObjectIndex::get_transform_node(const string& id) const
{
    return shared_ptr<rtr_format::TransformNode>();
}

shared_ptr<rtr_format::Light> ObjectIndex::get_light(const string& id) const
{
    return shared_ptr<rtr_format::Light>();
}

void ObjectIndex::insert_material(shared_ptr<rtr_format::Material> material)
{
    _material_map[material->id()] = material;
}

void ObjectIndex::insert_mesh(shared_ptr<rtr_format::Mesh> mesh)
{
    _mesh_map[mesh->id()] = mesh;
}

void ObjectIndex::insert_layer_source(shared_ptr<rtr_format::LayerSource> source)
{
    _layer_source_map[source->id()] = source;
}

void ObjectIndex::insert_animation(shared_ptr<rtr_format::Animation> animation)
{
    _animation_map[animation->id()] = animation;
}
