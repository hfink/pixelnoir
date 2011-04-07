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

#ifndef __TRANSFORM_H
#define __TRANSFORM_H

#include "common.h"
#include "AnimEvaluator.h"

class Transform;
//A Transform Node is a simple named collection 
//of transforms which are to be applied in their exact give sequence
//This single transformations could be animated as well
class TransformNode : noncopyable {

public:

    //dependency might be null as well
    //TODO: initialize matrices to identity!
    TransformNode(const string& id, const TransformNode * const dependency);
    virtual ~TransformNode();

    void add_lookat(const string& id,
                    const vec3& position,
                    const vec3& point_of_interest,
                    const vec3& up,
                    AnimEvaluator& evaluator);

    void add_matrix_transform(const string& id,
                              const mat4& matrix,
                              AnimEvaluator& evaluator);

    void add_rotate(const string& id,
                    const vec3& axis,
                    float angle,
                    AnimEvaluator& evaluator);

    void add_translate(const string& id,
                       const vec3& value,
                       AnimEvaluator& evaluator);

    void add_scale(const string& id,
                   const vec3& value,
                   AnimEvaluator& evaluator);

    void clear_transforms();

    void update();

    const mat4& get_matrix() const;
    const mat4& get_inverse_matrix() const;

    //Returns true, if the last update() call
    //changed the actual matrices
    bool has_changed() const;

    void override_transform(const mat4& matrix);

private:

    string _id;

    const TransformNode * const _dependency;

    vector<const Transform* > _transforms;

    mat4 _matrix;
    mat4 _inverse_matrix;

    bool _has_changed;

};

typedef shared_ptr<TransformNode> TransformNodeRef;

//base class for all transforms
//performs some caching as well...
//although the change information is more important
//for the culling system than actual calculation performance
class Transform {

public:
    
    virtual ~Transform() {}
    const mat4& get_matrix() const;

    virtual mat4 calculate_matrix() const = 0;
    virtual bool has_changed() const = 0;

protected:
    Transform(const string& id);

private:

    string _id;
    mutable mat4 _matrix_cache;

};

class LookAt : public Transform {

public:

    virtual mat4 calculate_matrix() const;
    virtual bool has_changed() const ;

    LookAt(const string& id,
            const vec3& position,
            const vec3& point_of_interest,
            const vec3& up,
            AnimEvaluator& evaluator);

private:

    AnimListener<vec3> _position;
    AnimListener<vec3> _point_of_interest;
    AnimListener<vec3> _up;
    
};

class MatrixTransform : public Transform {

public:

    MatrixTransform(const string& id,
                    const mat4& matrix,
                    AnimEvaluator& evaluator);

    virtual mat4 calculate_matrix() const;
    virtual bool has_changed() const ;

private:
    
    AnimListener<mat4> _matrix;
};

class Rotate : public Transform {

public:

    Rotate(const string& id,
           const vec3& axis,
           float angle,
           AnimEvaluator& evaluator);

    virtual mat4 calculate_matrix() const;
    virtual bool has_changed() const ;

private:
    AnimListener<vec3> _axis;
    AnimListener<float> _angle;

};

class Scale : public Transform {
    
public:

    Scale(const string& id,
          const vec3& value,
          AnimEvaluator& evaluator);

    virtual mat4 calculate_matrix() const;
    virtual bool has_changed() const ;

private:

    AnimListener<vec3> _value;

};

class Translate : public Transform {

public:
    Translate(const string& id,
              const vec3& value,
              AnimEvaluator& evaluator);
    
    virtual mat4 calculate_matrix() const;
    virtual bool has_changed() const ;

private:

    AnimListener<vec3> _value;

};

#endif //__TRANSFORM_H
