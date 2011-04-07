#ifndef __CAMERA_H
#define __CAMERA_H

#include "SceneObject.h"
#include "AnimEvaluator.h"
#include "common.h"

//fwd decl
class Frustum;

/**
 * Camera representation during runtime.
 * Note again that the camera will take over ownership the passed
 * AnimListeners.
 */
class Camera : public SceneObject {

public:

    enum FOV_AXIS {
        X_AXIS,
        Y_AXIS
    };

    Camera(const string& id,
           float fov_angle,
           FOV_AXIS fov_axis,
           float z_near,
           float z_far,
           AnimEvaluator& evaluator,
           const TransformNodeRef& node);

    virtual ~Camera() {}
    mat4 get_projection_matrix(float aspect) const;

    vec3 get_world_location() const;

    float get_y_fov() const;

    float get_z_near() const;
    void set_z_near(float val);

    float get_z_far() const;
    void set_z_far(float val);

    bool get_use_target() const;
    void set_use_target(bool use_target);

    float calculate_focus_depth() const;
    const TransformNodeRef& target() const;
    void set_target(const TransformNodeRef& target);

    void override_transform(const mat4& matrix);

    /**
     * Returns
     */
    Frustum get_frustum(float aspect) const;

private:
    
    const FOV_AXIS _fov_axis;

    AnimListener<float> _fov_angle;
    AnimListener<float> _z_near;
    AnimListener<float> _z_far;

    TransformNodeRef _target;

    bool _use_target;
};

/**
 * Represents a view-frustum with its six planes.
 */
class Frustum {
public: 

    static const int LEFT_PLANE   = 0;
    static const int RIGHT_PLANE  = 1;
    static const int BOTTOM_PLANE = 2;
    static const int TOP_PLANE    = 3;
    static const int NEAR_PLANE   = 4;
    static const int FAR_PLANE    = 5;

    Frustum(mat4 proj_view);

    const vec4& get_plane(int plane) const;

private:
    //Hesse normal forms of planes
    vec4 _planes[6];

};

typedef shared_ptr<Camera> CameraRef;

#endif //__CAMERA_H
