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

#ifndef RUNTIME_H
#define RUNTIME_H

#include "common.h"
#include "rtr_format.pb.h"
#include "Timer.h"
#include "Transform.h"
#include "Light.h"
#include "Camera.h"
#include "Geometry.h"
#include "AnimEvaluator.h"
#include "Mesh.h"
#include "UniformBuffer.h"
#include "LooseOctree.h"
#include "ObjectIndex.h"
#include "PostProcess.h"
#include "DustParticles.h"

class DBLoader;
class FBO;
class Viewport;
class GaussianBlur;

class Runtime
{
    public:

    //Note: ObjectIndex will be completely replaced once material loading, etc,
    //is fully implemented
    Runtime(const rtr_format::Scene& scene, 
            DBLoader* db_loader,
            const Viewport& viewport);

    ~Runtime();

    void update(const Timer& timer);
    void draw();

    void insert_node(const rtr_format::TransformNode& node);
    void insert_light(const rtr_format::Light& light);
    void insert_camera(const rtr_format::Camera& camera);
    void insert_geometry(const rtr_format::Geometry& geometry);
    void start_animation(const string& animation_id, float offset=0.0f);

    void reload_materials();

    void create_default_camera();

    void use_camera(const string& camera_name);

    const CameraRef& render_camera() const { return _render_camera; }
    const CameraRef& cull_camera() const { return _cull_camera; }

    void set_depth_of_field(bool enable);

    static const string& kObserverCameraName() {
        static const string s("__ObserverCam__");
        return s;
    }

    void toggle_observer_camera();

    DustParticles& get_particle_system() { return _dust_particles; }

    private:

    FBO* _fbo;
    Shader* _line_shader;
    GPUMeshRef _wired_cube;

    DBLoader* _db_loader;

    AnimEvaluator _evaluator;

    map<string, CameraRef> _cameras;
    map<string, GeometryRef> _geometries;
    map<string, LightRef> _lights;
    vector<TransformNodeRef> _nodes;
    map<string, TransformNodeRef> _node_map;
    map<string, GPUMeshRef> _meshes;
    map<string, GPULayerSourceRef> _sources;

    //TODO: this should be removed, we should not hold rtr_format datastructures,
    //however, animation evaluation needs it atm
    map<string, shared_ptr<rtr_format::Animation> > _animations_holder;

    CameraRef _render_camera;
    CameraRef _cull_camera;

    MaterialManager _material_manager;
    int _standard_program;
    UniformBuffer* _shared_UBO;
    UniformBuffer* _transform_UBO;

    LooseOctree* _octree;
    LooseOctree::QueryResult _octree_query;

    const Viewport& _viewport;

    PostProcess* _post_process;

    int _shadowmap_count;
    FBO* _shadow_fbo;
    vector<mat4> _shadowmap_matrices;

    Shader _shadow_shader;
    GaussianBlur* _shadow_blur;

    Texture _rgbz_buffer;

    DustParticles _dust_particles;

    GPUMeshRef get_mesh(const string& mesh_id);
    void create_observer_camera();
    void setup_octree();
    void clear_query(LooseOctree::QueryResult& octree_query); 
    void setup_shared_uniforms();
    void setup_transform_uniforms(UniformBuffer& transform,
                                  const mat4& model,
                                  const mat4& world,
                                  const mat4& projection);
    void draw_with_octree(const Frustum& frustum, int program);
    void draw_directly(int program);
    void draw_debug_info();
    void draw_cull_frustum();
    
    void draw_shadow(mat4 shadow_transform,
                     mat4 shadow_view);
    void draw_shadow_with_octree(const Frustum& frustum, 
                                 mat4 shadow_transform,
                                 mat4 shadow_view);
    void prepare_shadowmaps();
};

#endif
