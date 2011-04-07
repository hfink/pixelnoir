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

#include "Mesh.h"
#include "Shader.h"

#include "rtr_format.pb.h"

#include <fstream>
#include <limits>
using std::ios;

//helper method to determin actual GL type
GLenum primitive_to_gltype(const rtr_format::Mesh::PrimitiveType& pType) {
    switch(pType) {
        case rtr_format::Mesh::TRIANGLES:
            return GL_TRIANGLES;                    
        case rtr_format::Mesh::TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;                                      
        default:
            std::cerr << "Could not convert protocol buffer primitive type " 
                      << pType << " to GL type." << endl;
        return 0;
    }
}

bool gltype_to_primitive(GLenum gltype, rtr_format::Mesh::PrimitiveType& typeOut) {
    switch (gltype) {
    case GL_TRIANGLES:
        typeOut = rtr_format::Mesh::TRIANGLES;
        return true;
    case GL_TRIANGLE_STRIP:
        typeOut = rtr_format::Mesh::TRIANGLE_STRIP;
        return true;
    default:
        std::cerr << "Could not convert gl primitive type " << gltype 
                  << " to protocol buffer primitive type." << endl;
        return false;
    }
}

MeshInitializer::MeshInitializer(string id,
                                 GLenum primitive_type,
                                 GLint vertex_count) :
    _id(id),_primitive_type(primitive_type),
    _vertex_count(vertex_count) {};

MeshInitializer::MeshInitializer(const rtr_format::Mesh& mesh_buffer)
{

    _id = mesh_buffer.id();
    _primitive_type = primitive_to_gltype(mesh_buffer.primitive_type());
    _vertex_count = mesh_buffer.vertex_count();

    ArrayAdapter indices_data(mesh_buffer.index_data().data(), mesh_buffer.index_data_size());
    add_indices(indices_data);

    for (int i = 0; i < mesh_buffer.layer_size(); ++i) {

        const rtr_format::Mesh_VertexAttributeLayer& l = mesh_buffer.layer(i);

        //Note: At the moment we assume that all layers only carry
        //GL_FLOAT data types. At the moment this is a property on a
        //per-layer-source basis. We would have to pass this information
        //somewhow, if we allowed int layers as well.
        GLenum gl_type = GL_FLOAT;

        add_layer( l.name(), 
                   gl_type, 
                   l.num_components(),
                   l.source_index(),
                   l.source() );
    }

    //get the bounding volume data
    float b_sphere_radius = 0;
    vec3 b_sphere_center;

    b_sphere_center.x = mesh_buffer.bounding_sphere().center_x();
    b_sphere_center.y = mesh_buffer.bounding_sphere().center_y();
    b_sphere_center.z = mesh_buffer.bounding_sphere().center_z();

    b_sphere_radius = mesh_buffer.bounding_sphere().radius();

    _bounding_sphere = Sphere(b_sphere_radius, b_sphere_center);

}

void MeshInitializer::store(rtr_format::Mesh& mesh_buffer) const
{
    // Clear data in protocol buffer.
    mesh_buffer.Clear();
    
    // Copy all information into protocol buffer
    mesh_buffer.set_id(_id);
    rtr_format::Mesh::PrimitiveType prim_type;
    bool success = gltype_to_primitive(_primitive_type, prim_type);
    assert( success );

    mesh_buffer.set_primitive_type(prim_type);
    mesh_buffer.set_vertex_count(_vertex_count);

    const GLuint* indices = _index_data.access_elements<GLuint>();
    for (int i = 0; i < _index_data.size(); ++i) {
        mesh_buffer.add_index_data(indices[i]);
    }

    for (vector<LayerInfo>::const_iterator i = _layers.begin(); 
         i != _layers.end(); ++i) {
        rtr_format::Mesh_VertexAttributeLayer* l = mesh_buffer.add_layer();

        l->set_name(i->name);
        l->set_num_components(i->components);
        l->set_source(i->layer_source_id);
        l->set_source_index(i->source_index);

    }
}

void MeshInitializer::add_layer( string name, 
                                 GLenum type, 
                                 GLint components, 
                                 int source_index, 
                                 const string& layer_source_id )
{
    LayerInfo layer;

    layer.name = name;
    layer.type = type;
    layer.components = components;
        
    layer.layer_source_id = layer_source_id;
    layer.source_index = source_index;

    _layers.push_back(layer); 
}

void MeshInitializer::add_indices(const ArrayAdapter& index_array)
{
    _index_data = index_array;
}

int MeshInitializer::layers_size() const {
    return _layers.size();
}

const string& MeshInitializer::layer_source_id_at(int i) const {
    return _layers.at(i).layer_source_id;
}

GPUMesh::GPUMesh(const MeshInitializer& init, const GPULayerSourceMap& sources) :
    _id(init._id),
    _primitive_type(init._primitive_type),
    _vertex_count(init._vertex_count), 
    _index_count(init._index_data.size())
{

    const GLuint * idx_ptr = NULL;

    for (vector<MeshInitializer::LayerInfo>::const_iterator i = init._layers.begin();
         i != init._layers.end(); ++i) {
        LayerInfo layer;

        layer.name = i->name;
        layer.type = i->type;
        layer.components = i->components;

        //Find the converted source
        assert(sources.find(i->layer_source_id) != sources.end());

        GPULayerSourceRef source_data = sources.find(i->layer_source_id)->second;
        layer.source = source_data;

        layer.vbo_offset = (GLvoid*)( source_data->element_size()*i->source_index );

        _layers.push_back(layer);
    }

    // Set up index VBO if necessary
    if (_index_count > 0) {
        glGenBuffers(1, &_index_buffer);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
        
        idx_ptr = init._index_data.access_elements<GLuint>();

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*_index_count,
                     idx_ptr, GL_STATIC_READ);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    //Getbounding volume info
    _bounding_volume = BoundingVolume(init._bounding_sphere);

}

GPUMesh::~GPUMesh()
{   
    // Free index VBO if necessary
    if (_index_count > 0) {
        glDeleteBuffers(1, &_index_buffer);
    }

    // Delete all associated VAOs
    for (std::map<GLuint, GLuint>::iterator entry = _shader_to_vao_map.begin();
     entry != _shader_to_vao_map.end(); ++entry) {
      GLuint vao = entry->second;
      glDeleteVertexArrays(1, &vao);
    }
}

void GPUMesh::prepare_vao(const Shader& shader)
{
    // Delete old VAO for this shader if necessary
    if (_shader_to_vao_map.count(shader.get_program_ID()) > 0) {
        GLuint old_vao = _shader_to_vao_map[shader.get_program_ID()];
        glDeleteVertexArrays(1, &old_vao);
    }
    
    // Create and bind new VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Bind index VBO if necessary
    if (_index_count > 0)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);

    for (int i = 0; i < shader.get_attrib_count(); ++i) {
        string attrib_name = shader.get_attrib_name(i);
        
        list<LayerInfo>::const_iterator l = _layers.begin();
        while (l != _layers.end()) {

            if (attrib_name == l->name) break;
            
            ++l;
        }

        if (l == _layers.end()) { {
            cerr << "Error: Mesh is missing required vertex attribute '"
                 << attrib_name << "'!" << endl;
        }
            
        } else {

            GLint location = shader.get_attrib_location(attrib_name);
            
            //bind the VBO of the source
            l->source->bind();

            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, l->components,
                                  l->type, GL_FALSE, 0, l->vbo_offset);

            //unbind VBOs
            l->source->unbind();
        }         
    }

    glBindVertexArray(0);

    // Store vao index in map
    _shader_to_vao_map[shader.get_program_ID()] = vao;
}

void GPUMesh::draw(const Shader& shader)
{
    if (_shader_to_vao_map.count(shader.get_program_ID()) < 1) {
        prepare_vao(shader);
    }

    // This is actually trivial with VAOs.
    glBindVertexArray(_shader_to_vao_map.at(shader.get_program_ID()));

    if (_index_count > 0) {
        glDrawElements(_primitive_type, _index_count, GL_UNSIGNED_INT, NULL);
    } else {
        glDrawArrays(_primitive_type, 0, _vertex_count);
    }

    glBindVertexArray(0);
}
