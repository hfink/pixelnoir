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

#include "common.h"
#include "RtrPlayerConfig.h"

#include <fstream>
#include "Shader.h"

void calc_fps(float& fps, float& mspf)
{
    static double last = -1.0;
    static int frames = 0;
    static float current_fps = .0f;
    static float current_ms_per_frame = .0f;

    double now = glfwGetTime();
  
    if (last < 0.0) {
        last = now;
    }

    frames += 1;

    if (now - last >= 5.0) {
        printf("%.2f ms/frame (= %d fps)\n", 
              ((now-last)*1000.0/frames),
              (int)(frames/(now-last)));
        current_fps = float( frames/(now - last) );
        current_ms_per_frame = float( (now-last)*5000.0/frames );
        last = now;
        frames = 0;
    }

    fps = current_fps;
    mspf = current_ms_per_frame;
}

bool file_exists(const string &filename)
{
    std::ifstream ifile(filename.c_str());
    return ifile.good();
}

string read_file(const string &filename)
{
    std::ifstream ifile(filename.c_str());

    return string(std::istreambuf_iterator<char>(ifile),
        std::istreambuf_iterator<char>());
}

void get_errors(void)
{
    static int call_count = 0;

    // We don't need get_errors if we use ARB_debug_output
    if (!EXTGL_ARB_debug_output) {
        GLenum error = glGetError();

        if (error != GL_NO_ERROR) {
            if (call_count == 16) return;

            switch (error) {
            case GL_INVALID_ENUM:
                cerr << "GL: enum argument out of range." << endl;
                break;
            case GL_INVALID_VALUE:
                cerr << "GL: Numeric argument out of range." << endl;
                break;
            case GL_INVALID_OPERATION:
                cerr << "GL: Operation illegal in current state." << endl;
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                cerr << "GL: Framebuffer object is not complete." << endl;
                break;
            case GL_OUT_OF_MEMORY:
                cerr << "GL: Not enough memory left to execute command." << endl;
                break;
            default:
                cerr << "GL: Unknown error." << endl;
            }

            ++call_count;
        }
    }
}

/**
 * Callback method for ARB_debug_output.
 * @param source Source of the message.
 * @param type Type of the message.
 * @param id Identification number of the message.
 * @param severity Severity of the message.
 * @param length Length of the messge in bytes(?).
 * @param message Message.
 * @param userParam NULL.
 */
GLvoid APIENTRY opengl_debug_callback(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar* message,
                                      GLvoid* userParam)
{
    // A small hack to avoid getting the same error message again and again.
    static int call_count = 0;

    string source_name = "Unknown";
    string type_name = "Unknown";
    string severity_name = "Unknown";

    // Find source name
    switch (source) {
    case GL_DEBUG_SOURCE_API_ARB:
        source_name = "OpenGL"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        source_name = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        source_name = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        source_name = "External debuggers/middleware"; break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        source_name = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        source_name = "Other"; break;
    }

    // Find type name
    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
        type_name = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        type_name = "Deprecated Behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        type_name = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        type_name = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        type_name = "Performance"; break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        type_name = "Other"; break;
    }

    // Find severity name.
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        severity_name = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        severity_name = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        severity_name = "low"; break;
    }

    if (call_count < 8) {
        // Print all received message information.
        cerr << "Caught OpenGL debug message:" << endl;
        cerr << "\tSource:   " << source_name << endl;
        cerr << "\tType:     " << type_name << endl;
        cerr << "\tSeverity: " << severity_name << endl;
        cerr << "\tID:       " << id << endl;
        cerr << "\tMessage:  " << message << endl;
        cerr << endl;
    }

    ++call_count;
}

void set_GL_error_callbacks(void)
{
#ifdef DEBUG_OPENGL
    if (EXTGL_ARB_debug_output) {
        // Enable synchronous callbacks.
        // Otherwise we might not get usable stack-traces.
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    
        // Enable ALL debug messages.
        glDebugMessageControlARB(GL_DONT_CARE,
                                 GL_DONT_CARE,
                                 GL_DONT_CARE,
                                 0, NULL,
                                 GL_TRUE);

        // Set callback function
        glDebugMessageCallbackARB(opengl_debug_callback, NULL);
    }
#endif
}


#define CASE(enum) case enum: return #enum

const char* get_type_enum_name(GLenum type_enum) {
    switch(type_enum) {
        CASE(GL_FLOAT);
        CASE(GL_FLOAT_VEC2);
        CASE(GL_FLOAT_VEC3);
        CASE(GL_FLOAT_VEC4);
        CASE(GL_DOUBLE);
#ifdef GL_VERSION_4_0
        CASE(GL_DOUBLE_VEC2);
        CASE(GL_DOUBLE_VEC3);
        CASE(GL_DOUBLE_VEC4);
#endif
        CASE(GL_INT);
        CASE(GL_INT_VEC2);
        CASE(GL_INT_VEC3);
        CASE(GL_INT_VEC4);
        CASE(GL_UNSIGNED_INT);
        CASE(GL_UNSIGNED_INT_VEC2);
        CASE(GL_UNSIGNED_INT_VEC3);
        CASE(GL_UNSIGNED_INT_VEC4);
        CASE(GL_BOOL);
        CASE(GL_BOOL_VEC2);
        CASE(GL_BOOL_VEC3);
        CASE(GL_BOOL_VEC4);
        CASE(GL_FLOAT_MAT2);
        CASE(GL_FLOAT_MAT3);
        CASE(GL_FLOAT_MAT4);
        CASE(GL_FLOAT_MAT2x3);
        CASE(GL_FLOAT_MAT3x2);
        CASE(GL_FLOAT_MAT4x2);
        CASE(GL_FLOAT_MAT2x4);
        CASE(GL_FLOAT_MAT3x4);
        CASE(GL_FLOAT_MAT4x3);
#ifdef GL_VERSION_4_0
        CASE(GL_DOUBLE_MAT2);
        CASE(GL_DOUBLE_MAT3);
        CASE(GL_DOUBLE_MAT4);
        CASE(GL_DOUBLE_MAT2x3);
        CASE(GL_DOUBLE_MAT3x2);
        CASE(GL_DOUBLE_MAT4x2);
        CASE(GL_DOUBLE_MAT2x4);
        CASE(GL_DOUBLE_MAT3x4);
        CASE(GL_DOUBLE_MAT4x3);
#endif
    default:
        return "UNKNOWN";
    }
}


void debug_print_uniforms(Shader& shader) 
{
    GLint active_uniform_blocks;
    GLint max_block_name_length;
    GLint max_uniform_length;

    GLuint program = shader.get_program_ID();
    
    shader.bind();

    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &active_uniform_blocks);
    glGetProgramiv(program, 
                   GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, 
                   &max_block_name_length);
    glGetProgramiv(program, 
                   GL_ACTIVE_UNIFORM_MAX_LENGTH, 
                   &max_uniform_length);

    cout << active_uniform_blocks 
         << " active uniform buffer block(s)." << endl;

    char* uniform_block_name = new char[max_block_name_length];
    char* uniform_name = new char[max_uniform_length];
    for (int i = 0; i < active_uniform_blocks; ++i) {
        GLint uniform_count;

        glGetActiveUniformBlockiv(program, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, 
                                  &uniform_count);

        GLint* uniform_indices = new GLint[uniform_count];

        glGetActiveUniformBlockiv(program, i, 
                                  GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
                                  uniform_indices);

        glGetActiveUniformBlockName(program, (GLuint)i, max_block_name_length,
                                    NULL, uniform_block_name);

        cout << uniform_block_name << ": " << endl;

        for (int j = 0; j < uniform_count; ++j) {
            glGetActiveUniformName(program, uniform_indices[j], 
                                   max_uniform_length, NULL, uniform_name);
            cout << "\t" << uniform_name << endl;

            GLint type;
            GLint size;
            GLint offset;
            GLint array_stride;
            GLint matrix_stride;
            GLint is_row_major;

            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_TYPE, &type);
            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_SIZE, &size);
            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_OFFSET, &offset);
            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_ARRAY_STRIDE, &array_stride);
            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_MATRIX_STRIDE, &matrix_stride);
            glGetActiveUniformsiv(program, 1, (GLuint*)uniform_indices + j,
                                  GL_UNIFORM_IS_ROW_MAJOR, &is_row_major);
            
            cout << "\t\ttype: " << get_type_enum_name(type) << endl;
            cout << "\t\tsize: " << size << endl;
            cout << "\t\toffset: " << offset << endl;
            cout << "\t\tarray_stride: " << array_stride << endl;
            cout << "\t\tmatrix_stride: " << matrix_stride << endl;
            cout << "\t\trow_major: " << is_row_major << endl;
        }

        delete[] uniform_indices;
    }
    delete[] uniform_block_name;
    delete[] uniform_name;

    shader.unbind();
}
