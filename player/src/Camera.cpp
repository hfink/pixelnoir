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

#include "Camera.h"
#include "RtrPlayerConfig.h"
#include "common_const.h"

#include <glm/gtc/matrix_projection.hpp>

Camera::Camera(const string& id,
               float fov_angle,
               FOV_AXIS fov_axis,
               float z_near,
               float z_far,
               AnimEvaluator& evaluator,
               const TransformNodeRef& node) :
    SceneObject(id, node),
    _fov_axis(fov_axis),
    _fov_angle(evaluator, id+rtr::Targets::CAM_FOV_TARGET()),
    _z_near(evaluator, id+"_z_near"),
    _z_far(evaluator, id+"_z_far"),
    _use_target(true)
{
    _fov_angle.set(fov_angle);
    set_z_near(z_near);
    set_z_far(z_far);
}

mat4 Camera::get_projection_matrix(float aspect) const {
    return glm::perspective( get_y_fov(), 
                             aspect, 
                             _z_near.value(), 
                             _z_far.value() );
}

vec3 Camera::get_world_location() const {
    return vec3(this->get_local_to_world() * vec4(0, 0, 0, 1));
}

float Camera::get_y_fov() const {
    float y_fov = 0;
    if (_fov_axis == Camera::Y_AXIS) {
        y_fov = _fov_angle.value();
    } else {
        //Calculate from aspect ratio and x FOV using the relationship:
        //      aspect_ratio = tan(xFov/2) / tan(yFov/2)
        float aspect = config.aspect_ratio();
        float x_fov_radians = glm::radians(_fov_angle.value());
        y_fov = 2*glm::atan(glm::tan(x_fov_radians*0.5f)/aspect);
        y_fov = glm::degrees(y_fov);
    }

    return y_fov;
}

float Camera::get_z_near() const {
    return _z_near.value();
}

void Camera::set_z_near(float val) {
    _z_near.set(val);
}

float Camera::get_z_far() const {
    return _z_far.value();
}

void Camera::set_z_far(float val) {
    _z_far.set(val);
}

bool Camera::get_use_target() const {
    return _use_target;
}

void Camera::set_use_target(bool val) {
    _use_target = val;
}

float Camera::calculate_focus_depth() const {
    if (!_target || !_use_target) {
        return config.dof_default_focus();
    } else {
        //get normalized distance to focus plane
        vec4 target_location = _target->get_matrix()*vec4(0, 0, 0, 1);
        //transform to view space
        target_location = this->get_world_to_local()*target_location;
        //distance to plane is now simply the negative z value
        return -target_location.z;
    }
}

const TransformNodeRef& Camera::target() const {
    return _target;
}

void Camera::set_target(const TransformNodeRef& target) {
    _target = target;
}

Frustum Camera::get_frustum(float aspect) const {
    //calculate the current
    mat4 view_proj = get_projection_matrix(aspect) * 
                     SceneObject::get_world_to_local();
    Frustum f(view_proj);
    return f;
}

void Camera::override_transform(const mat4& matrix)
{
    transform_node()->override_transform(matrix);
}

Frustum::Frustum(mat4 m_pv) {

    //extract rows
    vec4 m0 = vec4(m_pv[0][0],m_pv[1][0], m_pv[2][0], m_pv[3][0]); 
    vec4 m1 = vec4(m_pv[0][1],m_pv[1][1], m_pv[2][1], m_pv[3][1]); 
    vec4 m2 = vec4(m_pv[0][2],m_pv[1][2], m_pv[2][2], m_pv[3][2]); 
    vec4 m3 = vec4(m_pv[0][3],m_pv[1][3], m_pv[2][3], m_pv[3][3]); 

    _planes[LEFT_PLANE]   = - (m3 + m0);
    _planes[RIGHT_PLANE]  = - (m3 - m0);
    _planes[BOTTOM_PLANE] = - (m3 + m1);
    _planes[TOP_PLANE]    = - (m3 - m1);
    _planes[NEAR_PLANE]   = - (m3 + m2);
    _planes[FAR_PLANE]    = - (m3 - m2);

    //normalize the planes
    for (int i = 0; i<6; ++i) {
        float lngth = glm::length(vec3(_planes[i]));
        _planes[i] = _planes[i]*(1.0f/lngth);
    }
}

const vec4& Frustum::get_plane(int plane) const {
    return _planes[plane];
}
