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

#ifndef SHADER_H
#define SHADER_H

#include "common.h"
#include "type_info.h"
#include "Texture.h"
#include "TextureArray.h"

#include "UniformBuffer.h"

/**
 * Represents a compiled GLSL shader.
 */
class Shader
{
    bool _valid; /**< State variable for testing compilation success */

    GLuint _program; /**< Shader program handle. */

    GLuint _vertex_shader; /**< Vertex shader handle. */
    GLuint _geometry_shader; /**< Geometry shader handle */
    GLuint _fragment_shader; /**< Fragment shader handle */
    GLuint _tess_ctrl_shader; /**< Tessellation control shader handle */
    GLuint _tess_eval_shader; /**< Tessellation evaluation shader handle */

    string _name;

    typedef boost::unordered_map<string, GLint> UniformMap;
    UniformMap _uniform_map;

    typedef boost::unordered_map<string, GLuint> UniformBlockMap;
    UniformBlockMap _uniform_block_map;

    public:
    
    /**
     * Load and compile a shader.
     * @param shader Name of the shader. Don't add a file-extension or dir.
     * @param material Name of the material. Optional.
     */
    Shader(const string& shader, const string& material="");
    ~Shader();

    /**
     * Bind the shader object
     */
    void bind() const
    {
        glUseProgram(_program);
    }

    /**
     * Unbind the shader object
     */
    void unbind() const
    {
#ifdef DEBUG_OPENGL
        glUseProgram(0);
#endif
    }

    /**
     * Set a uniform.
     * @param uniform_name Name of uniform.
     * @param value The value to set the uniform to.
     */
    template<typename T>
    void set_uniform(const string& uniform_name, const T& value)
    {
        UniformMap::iterator it = _uniform_map.find(uniform_name);
        
        if (it == _uniform_map.end())
            return;

        gltype_info<T>::set_uniform(it->second, value);
    }

    void set_uniform(const string& uniform_name, const Texture& texture)
    {
        UniformMap::iterator it = _uniform_map.find(uniform_name);
        
        if (it == _uniform_map.end())
            return;

        gltype_info<GLint>::set_uniform(it->second, texture.get_unit_number());
    }

    bool has_uniform(const string& uniform_name) const {
        return (_uniform_map.count(uniform_name) > 0);
    }

    void set_uniform(const string& uniform_name, const TextureArray& texture)
    {
        UniformMap::iterator it = _uniform_map.find(uniform_name);
        
        if (it == _uniform_map.end())
            return;

        gltype_info<GLint>::set_uniform(it->second, texture.get_unit_number());
    }

    void set_uniform_block(const string& block_name, 
                           const UniformBuffer& UBO) 
    {
        UniformBlockMap::iterator it = _uniform_block_map.find(block_name);
        
        if (it == _uniform_block_map.end())
            return;

        glUniformBlockBinding(_program, it->second, UBO.get_binding());
    };

    /**
     * Return the location of an attrib.
     */
    GLint get_attrib_location(const string& attrib_name) const
    {
        return glGetAttribLocation(_program, attrib_name.c_str());
    }

    /**
     * Return the shader program handle.
     */
    GLuint get_program_ID() const
    {
        return _program;
    }

    GLint get_attrib_count() const;

    string get_attrib_name(GLint index) const;

    /**
     * Used for checking if a shader has compiled correctly.
     */
    bool is_valid() const
    {
        return _valid;
    }
    
    private:

    void clean_up();
};

#endif
