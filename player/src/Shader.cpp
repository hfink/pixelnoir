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

#include "Shader.h"

#include <boost/regex.hpp>
#include <fstream>
#include "RtrPlayerConfig.h"

#define LOG_BUFFER_SIZE 1024*64

// Anonymous namespace
namespace {
/**
 * Print a shader program error log to the shell.
 * @param program OpenGL Shader Program handle.
 * @param filename Filename of the linked file(s) to give feedback.
 */
void program_log(GLuint program, const string& filename)
{
    char logBuffer[LOG_BUFFER_SIZE];
    GLsizei length;
  
    logBuffer[0] = '\0';
    glGetProgramInfoLog(program, LOG_BUFFER_SIZE, &length,logBuffer);
  
    if (length > 0) {
        cerr << "Failed to link program '" << filename << "'" << endl;
        cerr << logBuffer << endl;
    }
};

/**
 * Print a shader error log to shell.
 * @param program OpenGL Shader handle.
 * @param filename Filename of the compiled file to give feedback.
 */
void shader_log(GLuint shader, const string& filename)
{
    char logBuffer[LOG_BUFFER_SIZE];
    GLsizei length;
  
    logBuffer[0] = '\0';
    glGetShaderInfoLog(shader, LOG_BUFFER_SIZE, &length,logBuffer);

    if (length > 0) {
        cout << "Failed to compile shader '" << filename << "'" << endl;
        cout << logBuffer << endl;
    }
};

bool load_shader_source(const string& shader, 
                        const string& material,
                        const string& file_extension,
                        std::stringstream& ss)
{
    const int MAX_LENGTH = 1024;
    char buffer[MAX_LENGTH];

    const boost::regex include_pattern("@include\\s+<(.+)>.*");
    const boost::regex material_pattern("@material.*");
    boost::match_results<std::string::const_iterator> match;


    string shaderfile   = config.shader_dir()  +"/"+shader  +file_extension;
    string materialfile = config.material_dir()+"/"+material+file_extension;

    std::ifstream is(shaderfile.c_str());

    if (!is) {
        return false;
    }

    while (is.getline(buffer, MAX_LENGTH)) {
        std::string line(buffer);
        
        if (boost::regex_match(line, match, include_pattern)) {
            //Note: using match.str(1) instead of match[1] is a workaround
            //for VS2010
            string includefile = config.shader_dir()+"/"+match.str(1);

            if (!file_exists(includefile)) {
                cerr << "Could not find file " << includefile << endl;
                return false;
            }
            
            ss << read_file(includefile);
        } else if (boost::regex_match(line, match, material_pattern)) {
            
            if (!file_exists(materialfile)) {
                cerr << "Could not find file " << materialfile << endl;
                return false;
            }
            
            ss << read_file(materialfile);
        } else {
            ss << line << endl;
        }
    }

    return true;
}

/**
 * Load and compile a GLSL shader file.
 * @param filename GLSL file name.
 * @param type GLSL shader type enum.
 * @return Shader handle in case of success, 0 otherwise.
 */
GLuint compile_shader_object(const string& shader, 
                             const string& material,
                             GLenum type) 
{
    std::stringstream ss;

    string file_extension;

    switch(type) {
    case GL_VERTEX_SHADER: file_extension = ".vert"; break;
    case GL_GEOMETRY_SHADER: file_extension = ".geom"; break;
    case GL_FRAGMENT_SHADER: file_extension = ".frag"; break;
#ifdef GL_VERSION_4_0
    case GL_TESS_CONTROL_SHADER: file_extension = ".tess_ctrl"; break;
    case GL_TESS_EVALUATION_SHADER: file_extension = ".tess_eval"; break;
#endif
    default:
        assert(0);
    }

    if (!load_shader_source(shader, material, file_extension, ss)) {
        return 0;
    }

    string source = ss.str();

    GLuint shader_handle = glCreateShader(type);

    const char * csource = source.c_str();
    GLint source_length = source.size();

    glShaderSource(shader_handle, 1, &csource, &source_length);

    glCompileShader(shader_handle);

    GLint status;

    glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &status);
    
    if (status == GL_FALSE) {
        shader_log(shader_handle, shader+file_extension+"@"+material);

        // std::string line;
        // int line_no = 0;
        // while (std::getline(ss, line)) {
        //     cerr << ++line_no << " " << line << endl;
        // }

        glDeleteShader(shader_handle);
        return 0;
    }

    return shader_handle;
}
}

Shader::Shader(const string& shader,
               const string& material) :
    _valid(false),
    _program(0),
    _vertex_shader(0),
    _geometry_shader(0),
    _fragment_shader(0),
    _tess_ctrl_shader(0),
    _tess_eval_shader(0),
    _name(shader)
{

    // Load an compile shaders
    _vertex_shader = compile_shader_object(shader, material, GL_VERTEX_SHADER);
    _fragment_shader = compile_shader_object(shader, material, GL_FRAGMENT_SHADER);
    _geometry_shader = compile_shader_object(shader, material, GL_GEOMETRY_SHADER);


    // Test for errors
    if (_vertex_shader == 0 || _fragment_shader == 0) {
        clean_up();
        return;
    }

    _program = glCreateProgram();

    // Attach shaders to program
    glAttachShader(_program, _vertex_shader);
    glAttachShader(_program, _fragment_shader);

    if (_geometry_shader != 0) {
        glAttachShader(_program, _geometry_shader);
    }


#ifdef GL_VERSION_4_0
    _tess_ctrl_shader = compile_shader_object(shader, material,
                                              GL_TESS_CONTROL_SHADER);
    _tess_eval_shader = compile_shader_object(shader, material,
                                              GL_TESS_EVALUATION_SHADER);

    if (_tess_eval_shader != 0 && _tess_ctrl_shader != 0) {
        glAttachShader(_program, _tess_ctrl_shader);
        glAttachShader(_program, _tess_eval_shader);
    }
#endif

    // Link and check for success
    glLinkProgram(_program);

    GLint status;

    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    
    if (status == GL_FALSE) {
        program_log(_program, shader+"@"+material);

        clean_up();
        
        return;
    }

    _valid = true;

    GLint active_uniforms, active_uniform_blocks;

    glGetProgramiv(_program, GL_ACTIVE_UNIFORM_BLOCKS, &active_uniform_blocks);
    glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &active_uniforms);

    char buffer[1000];

    for (GLint uniform = 0; uniform < active_uniforms; ++uniform) {
        glGetActiveUniformName(_program, uniform, 1000, NULL, buffer);
        string uniform_name(buffer);
        _uniform_map[uniform_name] = glGetUniformLocation(_program, buffer);
    }

    for (GLuint uniform_block = 0; 
         uniform_block < GLuint(active_uniform_blocks); ++uniform_block) {
        glGetActiveUniformBlockName(_program, uniform_block, 
                                     1000, NULL, buffer);
        string uniform_block_name(buffer);
        _uniform_block_map[uniform_block_name] = uniform_block;
    }
}

Shader::~Shader()
{
    clean_up();
}

void Shader::clean_up()
{
    if (_program != 0) {
        glDeleteProgram(_program);
    }

    if (_vertex_shader != 0) {
        glDeleteShader(_vertex_shader);
    }

    if (_geometry_shader != 0) {
        glDeleteShader(_geometry_shader);
    }

    if (_fragment_shader != 0) {
        glDeleteShader(_fragment_shader);
    }

    if (_tess_ctrl_shader != 0) {
        glDeleteShader(_tess_ctrl_shader);
    }

    if (_tess_eval_shader != 0) {
        glDeleteShader(_tess_eval_shader);
    }

    _program = _vertex_shader = _geometry_shader = _fragment_shader = 0;
}

GLint Shader::get_attrib_count() const
{
    GLint attrib_count;
    glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &attrib_count);

    return attrib_count;
}

string Shader::get_attrib_name(GLint index) const
{
    assert(index < get_attrib_count());

    GLchar buffer[1000];
    GLint attrib_size;
    GLenum attrib_type;

    glGetActiveAttrib(_program, index, 1000, NULL, &attrib_size, &attrib_type, buffer);

    return string(buffer);
}
