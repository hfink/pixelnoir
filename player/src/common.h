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

#ifndef COMMON_H
#define COMMON_H

// ---------------------- COMMON HEADER FILE ---------------------- //

#define __STDC_LIMIT_MACROS

typedef unsigned char byte;

#include <ExtGL.h>
#define GLFW_NO_GLU
#include <GL/glfw.h>

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

// using glm::gtx::color_space::rgbColor;

#include <iostream>
#include <string>
#include <cstring>

#define _USE_MATH_DEFINES
#include <math.h>

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

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/utility.hpp>
#include <boost/any.hpp>
#include <boost/multi_array.hpp>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>

using boost::multi_array;
using boost::shared_ptr;
using boost::shared_array;
using boost::weak_ptr;
using boost::scoped_ptr;

using boost::noncopyable;
using boost::any;
using boost::any_cast;

#include "utility.h"

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

#endif //#ifndef COMMON_H
