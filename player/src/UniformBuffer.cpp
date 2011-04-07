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

#include "UniformBuffer.h"

#include "Shader.h"
#include <boost/regex.hpp>

UniformBuffer::BindingManager UniformBuffer::_binding_manager;

UniformBuffer::UniformBuffer(const Shader& shader,
                             const string& block_name) 
{
    GLuint program = shader.get_program_ID();
    GLint max_uniform_length;
    GLint uniform_count;
    
    glGetProgramiv(program, 
                   GL_ACTIVE_UNIFORM_MAX_LENGTH, 
                   &max_uniform_length);
    
    GLuint block_index = glGetUniformBlockIndex(program, block_name.c_str());
    glGetActiveUniformBlockiv(program, block_index, 
                              GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniform_count);

    char* uniform_name = new char[max_uniform_length];
    GLint* uniform_indices = new GLint[uniform_count];

    glGetActiveUniformBlockiv(program, block_index, 
                              GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                              uniform_indices);

    GLint uniform_block_size;

    glGetActiveUniformBlockiv(program, block_index,
                              GL_UNIFORM_BLOCK_DATA_SIZE,
                              &uniform_block_size);

    _buffer = new byte[uniform_block_size];

    _buffer_size = uniform_block_size;


    const boost::regex pattern(block_name+"\\.(.*)$");  // ..fourth
    boost::match_results<std::string::const_iterator> match;

    for (int i = 0; i < uniform_count; ++i) {
        glGetActiveUniformName(program, uniform_indices[i], 
                               max_uniform_length, NULL, uniform_name);
        
        GLint size;
        GLint offset;
        GLint array_stride;
        GLint matrix_stride;
        GLint is_row_major;

        glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + i,
                              GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + i,
                              GL_UNIFORM_OFFSET, &offset);
        glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + i,
                              GL_UNIFORM_ARRAY_STRIDE, &array_stride);
        glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + i,
                              GL_UNIFORM_MATRIX_STRIDE, &matrix_stride);
        glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + i,
                              GL_UNIFORM_IS_ROW_MAJOR, &is_row_major);

        Entry entry = {(size_t)offset,
                       (size_t)matrix_stride,
                       (is_row_major == GL_TRUE) ? true : false,
                       (size_t)array_stride,
                       (size_t)size};

        string uniform_name_str(uniform_name);
        if (boost::regex_match(uniform_name_str, match, pattern)) {
            _entries[string(match[1])] = entry;
        } else {
            _entries[uniform_name_str] = entry;
        }

    }

    delete[] uniform_name;
    delete[] uniform_indices;

    glGenBuffers(1, &_buffer_object);


    glBindBuffer(GL_UNIFORM_BUFFER, _buffer_object);

    glBufferData(GL_UNIFORM_BUFFER, _buffer_size, _buffer, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    _block_index = block_index;
    _buffer_binding = 0xffffffff;
}

void UniformBuffer::send_to_GPU()
{
    glBindBuffer(GL_UNIFORM_BUFFER, _buffer_object);

    glBufferSubData(GL_UNIFORM_BUFFER, 0, _buffer_size, _buffer);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

bool UniformBuffer::has_entry(const string& name) const {
    return (_entries.find(name) != _entries.end());
}

void UniformBuffer::bind()
{
    if (_buffer_binding == 0xffffffff) 
        _buffer_binding = _binding_manager.get_binding();

    glBindBufferBase(GL_UNIFORM_BUFFER, _buffer_binding, _buffer_object);
}

void UniformBuffer::unbind()
{
    if (_buffer_binding == 0xffffffff)
        return;

    glBindBufferBase(GL_UNIFORM_BUFFER, _buffer_binding, 0);

    _binding_manager.return_binding(_buffer_binding);
    _buffer_binding = 0xffffffff;

}

GLuint UniformBuffer::BindingManager::get_binding()
{
    if (_binding_list == NULL)
        initialize();

    if (_available_bindings < 1) {
        cerr << "UniformBuffer::BindingManager: "
             << "Ran out of available buffer bindings!! :<" << endl;
        return 0;
    }

    --_available_bindings;

    return _binding_list[_available_bindings];
}

void UniformBuffer::BindingManager::return_binding(GLuint binding)
{
    if (_binding_list == NULL)
        initialize();

    if (_available_bindings >= _max_bindings) {
        cerr << "UniformBuffer::BindingManager: "
             << "More buffer bindings returned than originally available!! o_O" << endl;
        return;
    }

    _binding_list[_available_bindings] = binding;

    ++_available_bindings;
}

void UniformBuffer::BindingManager::initialize()
{
    if (_binding_list != NULL) return;

    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, (GLint*)&_max_bindings);
    
    _binding_list = new GLuint[_max_bindings];
    _available_bindings = _max_bindings;

    for (GLuint i = 0; i < _max_bindings; ++i) {
        _binding_list[i] = _max_bindings-1-i;
    }
}

UniformBuffer::BindingManager::~BindingManager() {
    if (_binding_list != NULL)
        delete[] _binding_list;
}
