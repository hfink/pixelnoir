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

#ifndef _SCENE_OBJECT_H
#define _SCENE_OBJECT_H

#include "common.h"
#include "Transform.h"

class SceneObject {

public:

    SceneObject(const string& id, const TransformNodeRef& node);
    virtual ~SceneObject();

    const string& get_id() const;

    //bool is_static() const;

    const mat4& get_local_to_world() const;
    const mat4& get_world_to_local() const;

    const TransformNodeRef& transform_node() const { return _transform_node; }
private:

    inline static bool is_identity(const mat4& m) {

        static const mat4 id(1);

        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                if (m[c][r] != id[c][r]) return false;
        return true;
    }

    string _id;
    TransformNodeRef _transform_node;
};

#endif //_SCENE_OBJECT_H
