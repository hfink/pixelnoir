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

#include "Transform.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/transform2.hpp>
#include "common_const.h"

using namespace rtr;

TransformNode::TransformNode(const string& id, const TransformNode * const dependency) :
    _id(id), _dependency(dependency), _matrix(1.0f), _inverse_matrix(1.0f), _has_changed(true)
{
    //We immediately "update" this transform node in order to set this node
    //into a valid state
    update();
}

TransformNode::~TransformNode() {
    clear_transforms();
}

void TransformNode::add_lookat(const string& id,
                               const vec3& position,
                               const vec3& point_of_interest,
                               const vec3& up,
                               AnimEvaluator& evaluator) 
{
    _transforms.push_back(new LookAt(id, position, point_of_interest, up, evaluator));
}

void TransformNode::add_matrix_transform(const string& id,
                                         const mat4& matrix,
                                         AnimEvaluator& evaluator)
{
    _transforms.push_back(new MatrixTransform(id, matrix, evaluator));
}

void TransformNode::add_rotate(const string& id,
                               const vec3& axis,
                               float angle,
                               AnimEvaluator& evaluator) 
{							   
    _transforms.push_back(new Rotate(id, axis, angle, evaluator));
}

void TransformNode::add_translate(const string& id,
                                  const vec3& value,
                                  AnimEvaluator& evaluator) 
{
    _transforms.push_back(new Translate(id, value, evaluator));
}

void TransformNode::add_scale(const string& id,
                              const vec3& value,
                              AnimEvaluator& evaluator)
{
    _transforms.push_back(new Scale(id, value, evaluator));
}

void TransformNode::clear_transforms() {
    vector<const Transform *>::iterator it; 
    for (it=_transforms.begin(); it!=_transforms.end(); ++it) {
        delete (*it);
    }
}

bool TransformNode::has_changed() const {
    return _has_changed;
}

void TransformNode::update() {

    mat4 matrix = mat4(1.0f);
    mat4 inverse_matrix = mat4(1.0f);

    if (_dependency != NULL) {
        matrix = _dependency->get_matrix();
        inverse_matrix = _dependency->get_inverse_matrix();
    }

    //TODO: when animlistener correctly reports if updated
    //, then we can implement this much more efficiently...
    //for now we just compare resulting matrices in order
    //to test the culling system
    vector<const Transform*>::const_iterator it; 
    for (it=_transforms.begin(); it!=_transforms.end();++it) {

        mat4 m = (*it)->get_matrix();
        matrix = matrix * m;
        inverse_matrix = glm::inverse(m) * inverse_matrix;
        
    }

    //check if anything really changed
    if (matrix != _matrix) {
        _has_changed = true;
        _matrix = matrix;
        _inverse_matrix = inverse_matrix;
    } else {
        _has_changed = false;
    }

}

const mat4& TransformNode::get_matrix() const {
    return _matrix;
}

const mat4& TransformNode::get_inverse_matrix() const {
    return _inverse_matrix;
}

void TransformNode::override_transform(const mat4& matrix) 
{
    _matrix = matrix;
    _inverse_matrix = glm::inverse(matrix);
}

Transform::Transform(const string& id) :
    _id(id)
{}

const mat4& Transform::get_matrix() const {
    
    if (has_changed()) {
        //update cache
        _matrix_cache = calculate_matrix();
    }

    return _matrix_cache;
}

using namespace Targets;

LookAt::LookAt(const string& id,
               const vec3& position,
               const vec3& point_of_interest,
               const vec3& up,
               AnimEvaluator& evaluator) : 
        Transform(id),
        _position(evaluator, id+LOOKAT_POSITION_TARGET()),
        _point_of_interest(evaluator, id+LOOKAT_POINT_OF_INTEREST_TARGET()),
        _up(evaluator, id+LOOKAT_UP_TARGET())
{
    _position.set(position);
    _point_of_interest.set(point_of_interest);
    _up.set(up);
}


mat4 LookAt::calculate_matrix() const {

    const vec3& position = _position.value();
    const vec3& point_of_interest = _point_of_interest.value();
    const vec3& up = _up.value();

    return glm::inverse(glm::lookAt(position, point_of_interest, up));
}

bool LookAt::has_changed() const {
    //TODO: in this and all subsequent implementations, we would
    //rather have something like _position.has_changed(), that we set
    //by the animation system. then we could quickly test this in here
    return true;
}

MatrixTransform::MatrixTransform(const string& id,
                                 const mat4& matrix,
                                 AnimEvaluator& evaluator) :
    Transform(id),_matrix(evaluator, id)	
{
    _matrix.set(matrix);
}

mat4 MatrixTransform::calculate_matrix() const {
    return _matrix.value();
}

bool MatrixTransform::has_changed() const {
    //See LookAt::has_changed for comments
    return true;
}

Rotate::Rotate(const string& id,
              const vec3& axis,
              float angle,
              AnimEvaluator& evaluator) :
    Transform(id),
    _axis(evaluator, id+ROTATE_AXIS_TARGET()),
    _angle(evaluator, id+ROTATE_ANGLE_TARGET())
{
    _axis.set(axis);
    _angle.set(angle);
}

mat4 Rotate::calculate_matrix() const {

    const vec3& axis = _axis.value();
    float angle = _angle.value();

    return glm::rotate(angle, axis.x, axis.y, axis.z);

}

bool Rotate::has_changed() const {
    //wee LookAt::has_changed for comments
    return true;
}

Scale::Scale(const string& id,
             const vec3& value,
             AnimEvaluator& evaluator) :
    Transform(id),
    _value(evaluator, id)
{
    _value.set(value);
}
    
mat4 Scale::calculate_matrix() const {
    const vec3& value = _value.value();
    return glm::scale(value.x, value.y, value.z);
}

bool Scale::has_changed() const {
    //wee LookAt::has_changed for comments
    return true;
}

Translate::Translate(const string& id,
                     const vec3& value,
                     AnimEvaluator& evaluator) :
    Transform(id),
    _value(evaluator, id)
{
    _value.set(value);
}

mat4 Translate::calculate_matrix() const {
    const vec3& value = _value.value();
    return glm::translate(value.x, value.y, value.z);
}

bool Translate::has_changed() const {
    //wee LookAt::has_changed for comments
    return true;
}
