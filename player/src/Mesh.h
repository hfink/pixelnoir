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

#ifndef MESH_H
#define MESH_H

#include "common.h"
#include "type_info.h"
#include "LayerSource.h"
#include "ArrayAdapter.h"
#include "BoundingVolume.h"
#include <map>

#include <limits>

// fwd declarations
class GPUMesh;
namespace rtr_format {
    class Mesh;
}
class Shader;
/**
 * Application-side representation of mesh data.
 * This is an utility and storage class that is used to prepare a mesh for 
 * sending it the GPU. <br/>
 *
 * A MeshInitializer is safe to be copied, or returned from any method.
 */
class MeshInitializer {

    friend class GPUMesh;

public:

    /**
     * Create a new MeshInitializer object
     * @param primitive_type OpenGL primitive type.
     * @param vertex_count Number of vertices.
     */
    MeshInitializer(string id, GLenum primitive_type, GLint vertex_count);

    /**
     * Creates a new MeshInitializer from a protocol buffer's mesh format.
     * @param mesh_buffer The actual mesh buffer.
     * @param A map of already resolved layer sources, that could be referenced
     * by the mesh_buffer's layers.
     */
    MeshInitializer(const rtr_format::Mesh& mesh_buffer);

    /**
     * Store MeshInitializer data in a protocol buffer mesh.
     */
    void store(rtr_format::Mesh& mesh_buffer) const;

    /**
     * Add a new layer.
     * @param The name of this layer (as used in shaders).
     * @param source_index The starting index of this layer into the layer source data. This
     * index references nth element of the native data type of the layer source.
     * @param source The actual layer source data, as expressed by the layer's intermediate data
     * structure.
     */
    template<typename T> 
    void add_layer(string name, int source_index, 
                   const string& source);


    /**
     * Set index data.
     * @index_array The wrapped index array.
     */
    void add_indices(const ArrayAdapter& index_array);

    /**
     * Returns the count of layers of this mesh.
     */
    int layers_size() const;

    /**
     * Returns the layer source of a particular layer.
     */
    const string& layer_source_id_at(int i) const;

private:
       
    struct LayerInfo
    {
        string name;
        GLenum type;
        GLint components;
        string layer_source_id;
        int source_index; 
    };

    /**
     * Add a layer without type information.
     * @param name Layer name.
     * @param type OpenGL type enum.
     * @param components Number of components per vertex.
     * @param source_index The starting index of this layer into the layer source.
     * @param layer_source_id The id of the layer source element.
     */
    void add_layer( string name,
                    GLenum type, 
                    GLint components, 
                    int source_index, 
                    const string& layer_source_id);

    string _id;
    GLenum _primitive_type; /**< OpenGL primitive type enum */
    GLint _vertex_count; /**< Number of vertices */

    Sphere _bounding_sphere;
    //TODO: eventually, add OBB in here as well, but needs to be supported
    //in the rtr-format as well...

    //Array of layers.
    vector<LayerInfo> _layers;

    //Optional index data
    ArrayAdapter _index_data;

};

/**
 * GPU-side representation of mesh data.
 * This is basically represented by a VAO with one or more GPULayerSource (VBO)
 * backing storages.
 *
 * Note that this class is noncopyable, since destruction is
 * actually deleting the VAO online buffers.
 */ 
class GPUMesh : noncopyable {

public:

    /**
     * Creates a new GPU Mesh.
     * @param init MeshInitializer object used for initialization.
     * @param sources A map of possible sources as referenced by the
     * MeshInitializer. Note how these are already expressed as GPULayers.
     */
    GPUMesh(const MeshInitializer& init, 
            const GPULayerSourceMap& sources);

    /**
     * Destructor
     */
    ~GPUMesh();

    static GLuint PRIMITIVE_RESTART_IDX() {
        static const GLuint primitive_restart_idx = 0xffffffff;
        return primitive_restart_idx;
    }

    /**
     * Set up VAO for a specific shader.
     */
    void prepare_vao (const Shader& shader);

    /**
     * Draw the mesh for a specific shader.
     * This does not bind the shader. You have to do that yourself.
     */
    void draw (const Shader& shader);

    //returns the bounding volume for this mesh
    const BoundingVolume& bounding_volume() const { return _bounding_volume; }

private:

    struct LayerInfo
    {
        string name;
        GLenum type;
        GLint components;
        GPULayerSourceRef source;
        void* vbo_offset; 
    };

    string _id;
    GLenum _primitive_type;
    GLint _vertex_count, _index_count;
    GLuint _index_buffer;
    
    /**
     * This creates a mapping from shaders to individual vertex array objects.
     * This does not handle cleanup when a Shader object is deleted.
     * I don't think it's necessary because the memory consumption for VAOs is
     * negligible and shaders should life longer than meshes in most cases.
     */
    std::map<GLuint, GLuint> _shader_to_vao_map;

    /**
     * Layers
     */
    list<LayerInfo> _layers;

    BoundingVolume _bounding_volume;

};

typedef shared_ptr<GPUMesh> GPUMeshRef;

// TEMPLATE DEFINITIONS:
template<typename T> 
void MeshInitializer::add_layer(string name, int source_index, const string& source)
{
    add_layer(name, gltype_info<T>::type, gltype_info<T>::components, source_index, source);
}

#endif
