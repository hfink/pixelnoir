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

#include "LayerSource.h"

#include "rtr_format.pb.h"
#include "common.h"

#include <boost/math/special_functions/fpclassify.hpp>

GPULayerSource::GPULayerSource(const LayerSourceInitializer& init) {
    
    //Upload VBO data
    const ArrayAdapter& source_data = init.get_data();
    _num_elements = source_data.size();
    _id = init.get_id();
    const GLvoid* gl_read_ptr = NULL;

    if (source_data.access_elements<float>() != NULL) {
        _element_size = sizeof(float);
        gl_read_ptr = source_data.access_elements<float>();
    } else if (source_data.access_elements<int>() != NULL) {
        _element_size = sizeof(int);
        gl_read_ptr = source_data.access_elements<int>();
    } else {
        std::cerr << "Error type of source data in LayerSourceInitializer "
                  << "is unexpected." << std::endl;
        assert(NULL);
    }

    // Create attrib VBO
    glGenBuffers(1, &_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
    glBufferData( GL_ARRAY_BUFFER, 
                  _num_elements*_element_size, gl_read_ptr, GL_STATIC_READ);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GPULayerSource::~GPULayerSource() {
    //destroy vbo
    glDeleteBuffers(1, &_vertex_buffer);
}

void GPULayerSource::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
}
void GPULayerSource::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const string& GPULayerSource::get_id() const {
    return _id;
}
    
int GPULayerSource::num_elements() const {
    return _num_elements;
}

int GPULayerSource::element_size() const {
    return _element_size;
}

LayerSourceInitializer::LayerSourceInitializer() :
    _id("EMPTY")
{}

LayerSourceInitializer::
LayerSourceInitializer(const rtr_format::LayerSource& layer_source) :
    _id(layer_source.id())
{
    if (layer_source.type() == rtr_format::LayerSource::FLOAT) {
        _data = ArrayAdapter( layer_source.float_data().data(), 
                              layer_source.float_data_size());

#ifndef NDEBUG
        for (int i = 0; i < layer_source.float_data_size(); ++i) {
            float val = layer_source.float_data(i);
            if (!boost::math::isfinite(val)) {
                cerr << "Error: Layer source '" << layer_source.id() 
                     << "' contains non-finite data." << endl;
                break;
            }
        }
#endif
    } else if (layer_source.type() == rtr_format::LayerSource::INT32) {
        _data = ArrayAdapter(layer_source.int_data().data(),
                             layer_source.int_data_size());
    } else {
        std::cerr << "Error: protobuf layer source has an unexpected data type!" 
                  << std::endl;
        assert(NULL);
    }

    if (_data.get_gl_type() != GL_FLOAT) {
        cout << "Error: Layer source declares to hold non-float data. This " 
             << "is currently not supported." << endl;
    }
}

LayerSourceInitializer::LayerSourceInitializer( const string& id,
                                                const ArrayAdapter& data ) :
    _id(id), _data(data)
{}

const string& LayerSourceInitializer::get_id() const {
    return _id;
}

const ArrayAdapter& LayerSourceInitializer::get_data() const {
    return _data;
}

void LayerSourceInitializer::store(rtr_format::LayerSource& source) const
{
    source.set_id(_id);
 
    if (_data.get_gl_type() == GL_INT) {
        source.set_type(rtr_format::LayerSource::INT32);

        const GLint* elements = _data.access_elements<GLint>();
        for (int i = 0; i < _data.size(); ++i) {
            source.add_int_data(elements[i]);
        }
    } else {
        source.set_type(rtr_format::LayerSource::FLOAT);

        const GLfloat* elements = _data.access_elements<GLfloat>();
        for (int i = 0; i < _data.size(); ++i) {
            source.add_float_data(elements[i]);
        }
    }
}
