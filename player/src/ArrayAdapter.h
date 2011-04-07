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

#ifndef __ARRAY_ADAPTER_H
#define __ARRAY_ADAPTER_H

#include "common.h"
#include <glm/gtc/type_ptr.hpp>

/**
 * This class provides a convenient adapter (wrapper) to read-only arrays. The problem of
 * data ownership is implicitly given with the type of the "array holder". This holder
 * is represented by a boost::any instance, and therefore, could be any kind of readable
 * datasource. The only requirement is that we know how to extract const T* pointer
 * (by calling ArrayAdapter::access_elements) that provides us with a pointer to tightly
 * aligned memory data (which is required for example by OpenGL data uploading).
 *
 * We don't deal explicitly with ownership and assume that whoever creates an 
 * instance of this wrapper takes care of object construction/detruction in order
 * to have valid access to data. If we wrap this adapter, for example, around an
 * external data source, such as a protocol buffer, we assume that this adapter
 * won't live longer that its referenced data. If we can't assure that, create an 
 * instance of this wrapper around a shared_array copy. That way it is always safe to
 * access this wrapper's data.
 *
 * The nice thing about this wrapper is: 
 *   It is copyable (with or without implicit ownership)
 *   It correctly destructs anything that's wrapped in _array_holder, if necessary, and
 *   therefore correctly deal with reference counted data structures.
 *
 * The following data types can be read by this adapter: 
 * 
 *    T*
 *    const T*
 *
 * These types can be used when the source data holds the ownership, e.g. mesh generation
 * In these cases, the data is guaranteed to be valid during lifetime of this ArrayAdapte:
 *
 *    shared_array<T>
 *    shared_array<const T>
 *    shared_array<vec2>
 *    shared_array<vec3>
 *    shared_array<vec4>
 *
 */
class ArrayAdapter {

public:

	/**
	 * Creates an empty ArrayAdapter.
	 */
	ArrayAdapter();

	/**
	 * Creates an adapter around a boost::any instance, with the additional
	 * information about the size of the wrapped array. 
	 * Note that the size parameter always denotes the number of elements of the 
	 * actual wrapped type. If you create a wrapper like
	 *
	 * ArrayAdapter(some_array, 12)
	 *
	 * with some_array = shared_array<float>
	 *
	 * it means that this wrapper stores 12 elements of float data..
	 */
	ArrayAdapter(const any& mem, int size);

	/**
	 * Tries to access the array with a particular type. If you access
	 * this method with an invalid type, it will return NULL. That way
	 * you can test easily for a range of types.
	 * Note that this adapter only allows read access.
	 */
	template <typename T>
	const T* access_elements() const;

	/**
	 * Returns the size of elements that are readable from this wrapper. 
	 * This number is meant in terms of num_element of the actual type.
	 * You can query the type via the templated access_element method.
	 */
	int size() const;

	/**
	 * For a limited range of types, this convenience method returns
	 * the proper GL type, GL_INT for int arrays, and GL_FLOAT for float
	 * arrays.
	 */
	GLenum get_gl_type() const;

private:
	any _array_holder;
	int _size;

};

// inlucde inline and template definitions to keep this interface readable
#include "ArrayAdapter_Definition.h"

#endif //__ARRAY_ADAPTER_H