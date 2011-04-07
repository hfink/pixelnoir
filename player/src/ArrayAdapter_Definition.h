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

#ifndef __ARRAY_ADAPTER_DEFINITION_H
#define __ARRAY_ADAPTER_DEFINITION_H

#include "ArrayAdapter.h"

inline ArrayAdapter::ArrayAdapter() : 
	_array_holder(), _size(0)
{}

inline ArrayAdapter::ArrayAdapter(const any& mem, int size) :
	_array_holder(mem), _size(size)
{}

template <typename T>
const T* ArrayAdapter::access_elements() const {
		
	if (_array_holder.empty())
		return NULL;

	try {

		return any_cast<const T*>(_array_holder);
			
	} catch (const boost::bad_any_cast&) {}

	//actually T* array, have to explicitly cast to const T*
	try {
			
		return any_cast<T*>(_array_holder);
			
	} catch (const boost::bad_any_cast&) {}

	//wrapped in shared_array with const T
	try {
		//ok above didn't work, try it with a sharred_array wrapper
		shared_array<const T> access_ref = any_cast<shared_array<const T > >(_array_holder);
		return access_ref.get();
	} catch (const boost::bad_any_cast&) {}

	//wrapped in shared_array with T
	try {
		//ok above didn't work, try it with a sharred_array wrapper
		shared_array<T> access_ref = any_cast<shared_array<T > >(_array_holder);
		return access_ref.get();
	} catch (const boost::bad_any_cast&) {}

	try {
		shared_array<glm::detail::tvec2<T > > glm_vec2_access_ref = any_cast<shared_array<glm::detail::tvec2<T > > >(_array_holder);
		return glm::value_ptr(glm_vec2_access_ref[0]);
	} catch (const boost::bad_any_cast&) {}

	try {
		shared_array<glm::detail::tvec3<T > > glm_vec3_access_ref = any_cast<shared_array<glm::detail::tvec3<T > > >(_array_holder);
		return glm::value_ptr(glm_vec3_access_ref[0]);
	} catch (const boost::bad_any_cast&) {}

	try {
		shared_array<glm::detail::tvec4<T > > glm_vec4_access_ref = any_cast<shared_array<glm::detail::tvec4<T > > >(_array_holder);
		return glm::value_ptr(glm_vec4_access_ref[0]);
	} catch (const boost::bad_any_cast&) {}

	std::cerr << "Could not access elements. Don't know how to read." << std::endl;

	return NULL;
}

inline int ArrayAdapter::size() const { 
	return _size; 
}

inline GLenum ArrayAdapter::get_gl_type() const {
	if (access_elements<float>() != NULL)
		return GL_FLOAT;
	else if (access_elements<int>() != NULL)
		return GL_INT;
	else
		return 0;
}

#endif //__ARRAY_ADAPTER_DEFINITION_H