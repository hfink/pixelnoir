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

#ifndef __CB_COMMON_H
#define __CB_COMMON_H

//Shut up warnings from kyoto on Visual Studio 2010
#ifdef _MSC_VER
#pragma warning( disable : 4244)
#pragma warning( disable : 4351)
#pragma warning( disable:  4244)
#endif 

//We need to include the kyoto-header before including boost
#include <kchashdb.h>
#include <kcfile.h>

#ifdef _MSC_VER
#pragma warning( default : 4244)
#pragma warning( default : 4351)
#pragma warning( default:  4244)
#endif 


namespace kc = kyotocabinet;

//This is necessary to use the updated boost filesystem library

#include <iostream>
#include <string>
#include <cstring>

using std::string;
using std::cerr;
using std::cout;
using std::endl;

#include <list>
#include <vector>
#include <map>


using std::list;
using std::vector;
using std::map;

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>
#include <boost/any.hpp>
#include <boost/multi_array.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/regex.hpp>

using boost::multi_array;
using boost::shared_array;
using boost::noncopyable;
using boost::any;
using boost::any_cast;

#include <glm/setup.hpp>
#define GLM_SWIZZLE GLM_SWIZZLE_FUNC
#include <glm/glm.hpp>

#include <glm/gtx/color_space.hpp>
#include <glm/gtx/transform.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;

using glm::mat2;
using glm::mat4;
using glm::mat3;

using glm::cross;
using glm::dot;
using glm::normalize;
using glm::length;
using glm::rgbColor;

//For memory leak detection on Windows
#ifdef ENABLE_WIN_MEMORY_LEAK_DETECTION
    
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>

#ifdef _DEBUG
   #define DEBUG_CLIENTBLOCK   new( _CLIENT_BLOCK, __FILE__, __LINE__)
#else
   #define DEBUG_CLIENTBLOCK
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_CLIENTBLOCK
#endif

#endif

//we declare some namespace aliases to make code importer code shorter and more
//readable

namespace COLLADAFW {}

namespace CF = COLLADAFW;

namespace COLLADABU {}

namespace CB = COLLADABU;

namespace COLLADASaxFWL {}

namespace CS = COLLADASaxFWL;

//A subset of the main utility class. We copy it in here, because we don't 
//want all the other dependencies from utility.h (OGL & GLM, etc...)


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
#endif //__CB_COMMON_H
