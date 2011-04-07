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

#ifndef __LAYER_SOURCE_H
#define __LAYER_SOURCE_H

#include "common.h"
#include "ArrayAdapter.h"

namespace rtr_format {
    class LayerSource;
}
class LayerSourceInitializer;

/**
 * Runtime class for layer source data. This is basically equivalent to an
 * on-GPU VBO data storage, except that we still care about types. That means
 * If you wanted to access INT and FLOAT data with the same backing storage VBO, 
 * you would have to use two GPULayerSources. The OpenGL VBO itself does not 
 * care about data types, but we do, to keep the code case clean.
 *
 * Note that a layer source is initialized by a LayerSourceInitializer, similar
 * to Mesh and MeshInitializer.
 *
 * This class is tagged as being non-copyable, since its backing storage (VBO) 
 * is deleted on destruction. Use GPULayerSourceRef to pass instances around 
 * instead.
 */
class GPULayerSource : noncopyable {

public:

    GPULayerSource(const LayerSourceInitializer& init);
    ~GPULayerSource();

    //Binds the underlying VBO.
    void bind() const;

    //Unbind the VBO (actually, any VBO)
    void unbind() const;

    //This layer's ID
    const string& get_id() const;
    
    /**
     * Returns the number of elements that fit into this VBO.
     */
    int num_elements() const;

    /**
     * Return the size in number of bytes per elements.
     */
    int element_size() const;

private:

    GLuint _vertex_buffer;
    int _num_elements;
    int _element_size;
    string _id;

};

typedef shared_ptr<GPULayerSource> GPULayerSourceRef;
typedef map<string, GPULayerSourceRef> GPULayerSourceMap;

class LayerSourceInitializer;
typedef map<string, LayerSourceInitializer> LayerSourceInitializerMap;

/**
 * This initializer class is an intermediate storage and
 * convenience wrapper that is used to initialize a GPULayer.
 * It could be filled either by external data sources, or manually
 * such as used by mesh generation.
 */
class LayerSourceInitializer {

    friend class GPULayerSource;

public:	

    LayerSourceInitializer();

    /**
     * Creates a layer source initializer based on the protocol buffer's
     * format.
     */
    LayerSourceInitializer(const rtr_format::LayerSource& layer_source);

    /**
     * Creates layer source initializer based on any array data source.
     */
    LayerSourceInitializer(const string& id, const ArrayAdapter& data);

    /**
     * The ID of this layer, used to resolve dependencies on construction with
     * GPUMeshes.
     */
    const string& get_id() const;

    /**
     * The backing data of this layer sources.
     */
    const ArrayAdapter& get_data() const;

    void store(rtr_format::LayerSource& source) const;

    /**
     * Convenenience method to convert a map of Layer Source Initializer objects
     * in batch mode to a map of GPU layer source objects. Note that this method
     * therefore already upstreams GPU data!.
     */
    inline static GPULayerSourceMap to_gpu_layer_map(const LayerSourceInitializerMap& m)
    {
        GPULayerSourceMap m_result;
        LayerSourceInitializerMap::const_iterator it;
        for (it = m.begin(); it != m.end(); ++it) {
            m_result[it->second.get_id()] = 
                GPULayerSourceRef(new GPULayerSource(it->second));
        }
    
        return m_result;
    }

private:

    string _id;
    ArrayAdapter _data;

};


#endif //__LAYER_SOURCE_H
