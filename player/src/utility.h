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

#ifndef UTILITY_H
#define UTILITY_H

#include "common.h"
#include <sstream>
#include <iomanip>

const char* get_type_enum_name(GLenum type_enum);

void calc_fps(float& fps, float& mspf);

/**
 * Test if a file exists on the filesystem.
 * @param filename Name of the file
 * @return True if the file exists.
 */
bool file_exists(const string &filename);

/**
 * Read the content of a text file.
 * @param filename Name of the file.
 * @return String with the whole content of the file.
 */
string read_file(const string &filename);


/**
 * Check for OpenGL errors.
 * Prints a message to STDERR if an error has been found.
 */
void get_errors(void);


/**
 * Attempt to set ARB_debug_output callback.
 * This only does something when usign the Debug build.
 */
void set_GL_error_callbacks(void);

/**
 * Parse a string variable and store inside a statically typed variable.
 * This is the default implementation which should work with types that 
 * implement C++ stream operators.
 * @param s Parsed input string.
 * @param t Output parameter for parsed variable.
 * @return true in case of success, false otherwise.
 */
template<typename T> inline bool from_string(const string& s, T& t)
{
    std::stringstream ss(s);
    
    T tmp;

    if ((ss >> tmp).fail()) {
        return false;
    }
    
    t = tmp;
    return true;
}

/**
 * Convert an arbitrary type variable to a string.
 * This is the default implementation which should work with types that 
 * implement C++ stream operators.
 * @param t Input value.
 * @return The string representing the value.
 */
template<typename T> inline string to_string(const T& t)
{
    std::stringstream ss;
    
    ss << t;

    return ss.str();    
}

/**
 * Parse a string variable and store inside a statically typed variable.
 * This is a trivial implementation for strings which simply creates a copy.
 */
template<> inline bool from_string(const string& s, string& t)
{
    t = s;
    
    return true;
}

/**
 * Convert an arbitrary type variable to a string.
 * This is a trivial implementation for strings which simply creates a copy.
 */
template<> inline string to_string(const string& t)
{
    return t;
}

template<> inline bool from_string(const string& s, bool& b) {
    if (s == "true") {
        b = true;
        return true;
    } else if (s == "false") {
        b = false;
        return true;
    }

    return false;
}

template<> inline string to_string(const bool& b)
{
    if (b == true) {
        return "true";
    } else {
        return "false";
    }
}

template<> inline bool from_string(const string& s, vec3& v) {

    std::stringstream ss(s);

    float x,y,z;

    if ((ss >> x).fail() || (ss >> y).fail() || (ss >> z).fail()) {
        return false;
    } else {
    }


    v = vec3(x,y,z);
    return true;
}

template<> inline string to_string(const vec3& v)
{
    std::stringstream ss;

    ss << v.x << " " << v.y << " " << v.z;

    return ss.str();
}

template<> inline bool from_string(const string& s, vec4& v) {

    std::stringstream ss(s);

    float x,y,z,w;

    if ((ss >> x).fail() || (ss >> y).fail() || 
        (ss >> z).fail() || (ss >> w).fail()) {
        return false;
    } else {
    }


    v = vec4(x,y,z,w);
    return true;
}

template<> inline string to_string(const vec4& v)
{
    std::stringstream ss;

    ss << v.x << " " << v.y << " " << v.z << " " << v.w;

    return ss.str();
}

namespace rtr
{
  template <typename T> T min (T a, T b)
    {
      return a < b ? a : b;
    }

  template <typename T> T max (T a, T b)
    {
      return a < b ? b : a;
    }
}

template <typename T>
inline std::ostream& operator<< (std::ostream& out, const glm::detail::tvec3<T >& v) {
    out << std::setprecision(2) << std::fixed << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return out;
}

template <typename T>
inline std::ostream& operator<< (std::ostream& out, const glm::detail::tvec4<T >& v) {
    out << std::setprecision(2) << std::fixed << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return out;
}

template <typename T>
inline std::ostream& operator<< (std::ostream& out, const glm::detail::tmat4x4<T >& v) {
    out << std::setprecision(2) << std::fixed;
    for (int i = 0; i<4; ++i) {
        for (int j = 0; j<4; ++j) {
            out << v[j][i] << " ";
        }
        out << endl;
    }
    return out;
}

inline bool epsilon_compare(float a, float b) {
    static const float EPSILON = 0.001f;
    return ( ((b-EPSILON) < a) &&
                ((b+EPSILON) > a) );
}

inline bool operator==(const mat4& lhs, const mat4& rhs) {

    bool equal = true;
    for (unsigned int i = 0; i<4; i++) {
        for (unsigned int j = 0; j<4; j++) {
            equal &= (epsilon_compare(lhs[i][j], rhs[i][j]) );
        }
    }
    return equal;
}

inline bool operator!=(const mat4& lhs, const mat4& rhs) {
    return !operator==(lhs, rhs);
}
#endif //#ifndef _UTILITY_H_
