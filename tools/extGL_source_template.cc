// WARNING: This file was automatically generated
// Do not edit.

$('#include "%s.%s"' % ($options.outfilename, $options.headerext))
#raw
#include "GL/glfw.h"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
#end raw

void load_opengl_functions();

int init_opengl() 
{
    load_opengl_functions();

    int minor = glfwGetWindowParam(GLFW_OPENGL_VERSION_MINOR);
    int major = glfwGetWindowParam(GLFW_OPENGL_VERSION_MAJOR);

    if (major < $version.major || (major == $version.major && minor < $version.minor)) {
        cerr << "Error: OpenGL version $(version.major).$(version.minor) not supported." << endl;
        cerr << "       You version is " << major << "." << minor << "." << endl;
        cerr << "       Try updating your graphics driver." << endl;
        return GL_FALSE;
    }

#if $version.major >= 3 and $version.minor >=2
    GLint profile;
    
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

#if $version.core
    if ((profile & GL_CONTEXT_CORE_PROFILE_BIT) == 0) {
        cerr << "Error: This application requires a core profile" << endl;
        return GL_FALSE;
    }
#elif $version.major >= 3 and $version.minor >= 2
    if ((profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0) {
        cerr << "Error: This application requires a compatiblity profile" << endl;
        return GL_FALSE;
    }
#end if
#end if

#for $extension,$required in $extensions.iteritems()
    
#if $required:
    if (!glfwExtensionSupported("GL_$(extension)")) {
        cerr << "Error: OpenGL extension $(extension) not supported." << endl;
        cerr << "       Try updating your graphics driver." << endl;
        return GL_FALSE;
    }
#else
    if (glfwExtensionSupported("GL_$(extension)")) {
        EXTGL_$extension = GL_TRUE;
    }
#end if   
#end for

    return GL_TRUE;
}

void load_opengl_functions()
{
    // --- Function pointer loading
#for $category in $categories
#if $functions.has_key($category) and len($functions[$category]) > 0 and $category not in ['VERSION_1_0', 'VERSION_1_1','VERSION_1_0_DEPRECATED', 'VERSION_1_1_DEPRECATED' ]

    // $category

#for $function in $functions[$category]
    glpf$function.name = (PFNGL$(function.name.upper())PROC)glfwGetProcAddress("gl$function.name");
#end for
#end if
#end for
}

// ----------------------- Extension flag definitions ---------------------- 
#for $extension,$required in $extensions.iteritems()
#if not $required
int EXTGL_$extension = GL_FALSE;
#end if
#end for

// ----------------- Function pointer definitions ----------------

#for $category in $categories
#if $functions.has_key($category) and len($functions[$category]) > 0 and $category not in ['VERSION_1_0', 'VERSION_1_1','VERSION_1_0_DEPRECATED', 'VERSION_1_1_DEPRECATED' ]
#for $function in $functions[$category]
PFNGL$(function.name.upper())PROC glpf$(function.name) = NULL;
#end for
#end if
#end for
