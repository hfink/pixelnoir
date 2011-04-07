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

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#include "common.h"
#include "type_info.h"

class Shader;

class UniformBuffer
{
    struct Entry
    {
        size_t offset;
        size_t matrix_stride;
        bool matrix_is_row_major;
        size_t array_stride;
        size_t array_size;
    };

    byte* _buffer;
    size_t _buffer_size;
    map<string, Entry> _entries;
    GLuint _block_index;
    GLuint _buffer_object;
    GLuint _buffer_binding;

    public:

    UniformBuffer(const Shader& shader, 
                  const string& block_name);
    ~UniformBuffer() 
    { 
        delete[] _buffer;
    }

    template<typename T> void set(const string& name, const T& value) 
    {
        Entry& entry = _entries.at(name);

        gltype_info<T>::set_memory_location(value, _buffer + entry.offset,
                                            entry.matrix_stride,
                                            entry.matrix_is_row_major);
    }

    template<typename T> void set(const string& name, int index, 
                                  const T& value) 
    {
        Entry& entry = _entries.at(name);
        
        size_t offset = entry.offset + index * entry.array_stride;

        gltype_info<T>::set_memory_location(value, _buffer + offset,
                                            entry.matrix_stride,
                                            entry.matrix_is_row_major);
    }
    
    void bind();
    void unbind();
    void send_to_GPU();

    bool has_entry(const string& name) const;

    GLuint get_binding() const 
    { 
        assert(_buffer_binding != 0xffffffff);
        return _buffer_binding; 
    }

    private:
                 
    /**
     * Helper class for automatic buffer-binding management.
     */
    class BindingManager
    {
        GLuint* _binding_list; /**< Stack with unused buffer bindings. */
        GLuint _available_bindings; /**< Number of available bindings on stack. */
        GLuint _max_bindings; /**< Total size of stack. */

        public:

        GLenum get_binding(); /**< Acquire unused buffer binding */
        void return_binding(GLenum binding); /**< Return buffer binding */

        BindingManager() :
            _binding_list(NULL) {}
        ~BindingManager();

        private:

        /**
         * Call this function during Buffer construction.
         */
        void initialize();
    };

    /**
     * Buffer binding manager
     */
    static BindingManager _binding_manager;
};

#endif
