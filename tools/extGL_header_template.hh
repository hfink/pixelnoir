#raw
// WARNING: This file was automatically generated
// Do not edit.

#ifndef __gl_h_
#define __gl_h_

#ifdef __cplusplus
extern "C" {
#endif

/* Function declaration macros - to move into glplatform.h */

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

#end raw

//-------------------------------- DATA TYPES -------------------------------//

typedef unsigned int	GLenum;
typedef unsigned char	GLboolean;
typedef unsigned int	GLbitfield;
typedef void		GLvoid;
typedef signed char	GLbyte;		/* 1-byte signed */
typedef short		GLshort;	/* 2-byte signed */
typedef int		GLint;		/* 4-byte signed */
typedef unsigned char	GLubyte;	/* 1-byte unsigned */
typedef unsigned short	GLushort;	/* 2-byte unsigned */
typedef unsigned int	GLuint;		/* 4-byte unsigned */
typedef int		GLsizei;	/* 4-byte signed */
typedef float		GLfloat;	/* single precision float */
typedef float		GLclampf;	/* single precision float in [0,1] */
typedef double		GLdouble;	/* double precision float */
typedef double		GLclampd;	/* double precision float in [0,1] */

//--------------------------------- PASSTHRU --------------------------------//

$passthru

//----------------------------------- ENUMS ---------------------------------//

enum extGLenum {
#for $enum,$value in $enums
    GL_$enum $(' ' * ($longest_enum-len($enum))) = $hex($value),
#end for
};
//------------------------ FUNCTION PROTOTYPES -----------------------------//

#for $category in $categories
#if $functions.has_key($category) and len($functions[$category]) > 0

// $category

#if $category in ['VERSION_1_0', 'VERSION_1_1','VERSION_1_0_DEPRECATED', 'VERSION_1_1_DEPRECATED' ]
#for $function in $functions[$category]
GLAPI $(function.returntype) APIENTRY gl$(function.name) ($(', '.join(['%s %s' % (type,name) for name, type in $function.params])));
#end for
#else
#for $function in $functions[$category]
typedef $(function.returntype) (APIENTRYP PFNGL$(function.name.upper())PROC)($(', '.join(['%s %s' % (type,name) for name, type in $function.params])));
#end for

#for $function in $functions[$category]
GLAPI PFNGL$(function.name.upper())PROC glpf$function.name;
#end for

#for $function in $functions[$category]
#define gl$function.name glpf$function.name
#end for
#end if
#end if
#end for

#for $category in $categories
#define GL_$category 1
#end for

#ifdef __cplusplus
}
#endif

int init_opengl();

#define EXTGL_MAJOR_VERSION $(version.major)
#define EXTGL_MINOR_VERSION $(version.minor)
#define EXTGL_CORE_PROFILE $(1 if $version.core else 0)

// ------------------ Flags for optional extensions ---------------- 
#for $extension,$required in $extensions.iteritems()
#if not $required
extern int EXTGL_$extension;
#end if
#end for

#endif /* __gl_h_ */
