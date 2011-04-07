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

#include "DBLoader.h"

#include "Runtime.h"
#include "RtrPlayerConfig.h"
#include "Viewport.h"

#include "FBO.h"
#include "Shader.h"

#include "mesh_generation.h"

#include <glm/gtc/matrix_projection.hpp>

#include <limits>

#include "TextureArray.h"

#include "GaussianBlur.h"

Runtime::Runtime(const rtr_format::Scene& scene,
                 DBLoader* db_loader,
                 const Viewport& viewport) :
    _db_loader(db_loader), 
    _material_manager(), 
    _octree(NULL), 
    _viewport(viewport),
    _shadowmap_count(0),
    _shadow_shader("shadow"),
    _rgbz_buffer(2, viewport.render_size().x, viewport.render_size().y, 0,
                 GL_RGBA, GL_RGBA16F, 
                 GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE),
    _dust_particles(config.dust_center(), config.dust_size(), 
                    viewport.render_size())
{
    _standard_program = _material_manager.add_shader_program("standard");

    for (int i = 0; i < scene.node_size(); ++i) {
        insert_node(scene.node(i));
    }

    //AMD drivers need texture layer indices for FBO targets in ascending order
    //(out of some curious reason). Therefore shadow_map have to be created
    //in the same order they are iterated later (by ordered name). Therefore
    //we sort before creating the runtime data structures.

    map<string, const rtr_format::Light *> presort_lights;

    for (int i = 0; i < scene.light_size(); ++i) {
        presort_lights[scene.light(i).id()] = &scene.light(i);
    }

    map<string, const rtr_format::Light *>::const_iterator it_light;
    for (it_light = presort_lights.begin();
         it_light != presort_lights.end();
         ++it_light)
    {
        insert_light(*it_light->second);
    }

    for (int i = 0; i < scene.geometry_size(); ++i) {
        insert_geometry(scene.geometry(i));
    }

    for (int i = 0; i < scene.camera_size(); ++i) {
        insert_camera(scene.camera(i));
    }

    for (int i = 0; i < scene.animation_size(); ++i) {
        start_animation(scene.animation(i), config.animation_offset());
    }

    setup_octree();

    // Pick the first shader from the material manager.
    // Since all material-shaders have to support the shared and transform
    // UBO, it doesn't matter which one we use. We expect that there is at 
    // least one shader program and one material in use.

    Shader& some_shader = _material_manager.get_shader(0,0);
    _shared_UBO = new UniformBuffer(some_shader, "Shared");
    _transform_UBO = new UniformBuffer(some_shader, "Transform");

    //if we haven't imported any cameras, we must create a default one
    if (_cameras.empty()) {
        create_default_camera();
    }

    use_camera(_cameras.begin()->first);

    create_observer_camera();

    FBOFormat format;
    format.add_renderbuffer(GL_DEPTH_COMPONENT32, GL_DEPTH_ATTACHMENT);
    format.add_renderbuffer(GL_RGBA16F, GL_COLOR_ATTACHMENT0);

    int fsaa_samples = (config.fsaa_samples() == 1) ? 0 : config.fsaa_samples();
    _fbo = new FBO(_viewport.render_size(), fsaa_samples, format);

    _line_shader = new Shader("line");

    //A unit cube as a line loop is always useful and doesn't hurt
    LayerSourceInitializerMap lyr_init_map;
    MeshInitializer cube_mesh = create_wired_cube("cube_mesh_init",lyr_init_map);

    GPULayerSourceMap gpu_lyr_map = 
        LayerSourceInitializer::to_gpu_layer_map(lyr_init_map);

    _wired_cube = GPUMeshRef(new GPUMesh(cube_mesh, gpu_lyr_map));

    if (config.use_depth_of_field()) {
        _post_process = new DepthOfFieldPostProcess(_viewport);
    } else {
        _post_process = new SimplePostProcess(_viewport);
    }

    FBOFormat shadow_format;
    shadow_format.add_renderbuffer(GL_DEPTH_COMPONENT32, GL_DEPTH_ATTACHMENT);

    shadow_format.add_texture_array(_shadowmap_count, 
                                    GL_RG32F, GL_COLOR_ATTACHMENT0,
                                    GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, 
                                    GL_CLAMP_TO_EDGE);

    int shadow_size = config.shadowmap_size();

    _shadow_fbo = new FBO(ivec2(shadow_size), 0, shadow_format);

    if (config.shadowmap_blur_radius() > 0) {
        int blur_radius = int(config.shadowmap_blur_radius()/512.0 * shadow_size)+1;

        _shadow_blur = new GaussianBlur(blur_radius, 
                                        ivec2(config.shadowmap_size()), GL_RG32F);
    } else {
        _shadow_blur = NULL;
    }
    
    cout << "Shadowmap count: " << _shadowmap_count << endl;
}

Runtime::~Runtime()
{
    delete _octree;
    delete _shared_UBO;
    delete _transform_UBO;
    delete _fbo;
    delete _shadow_fbo;
    delete _line_shader;
    
    if (_shadow_fbo != NULL) {
        delete _shadow_blur;
    }

    delete _post_process;
}

void Runtime::create_default_camera() 
{
    //Create the transform
    TransformNodeRef new_node(new TransformNode("DEFAULT_CAM_NODE", NULL));

    new_node->add_lookat( "default_cam_lookat", 
                          vec3(10, 0, 0), 
                          vec3(0, 0, 0), 
                          vec3(0, 0, 1),
                          _evaluator );

    _node_map["DEFAULT_CAM_NODE"] = new_node;

    _nodes.push_back(new_node);

    _cameras["DEFAULT_CAM"] = CameraRef(new Camera("DEFAULT_CAM", 
                                                    45.0f,
                                                    Camera::Y_AXIS,
                                                    0.01f,
                                                    1000.0f,
                                                    _evaluator, 
                                                    new_node ));

}

void Runtime::create_observer_camera() {
    //This is a special camera which is used to externally observe
    //view frustum culling.

    TransformNodeRef obs_cam_node(new TransformNode(kObserverCameraName()+"_node", 
                                                    _render_camera->transform_node().get()));

    obs_cam_node->add_lookat("__observer_look_at", 
                             vec3(0, config.observer_cam_height(), 0), 
                             vec3(0, 0, -config.observer_cam_focus_offset()), vec3(0, 0, -1),
                             _evaluator);

    _nodes.push_back(obs_cam_node);

    _cameras[kObserverCameraName()] = CameraRef(new Camera(kObserverCameraName(), 
                                                           45.0f,
                                                           Camera::Y_AXIS,
                                                           0.5f,
                                                           1000.0f,
                                                           _evaluator, 
                                                           obs_cam_node ));
}

void Runtime::toggle_observer_camera() {
    if (_render_camera->get_id() == kObserverCameraName()) {
        //switch back
        _render_camera = _cull_camera;
        cout << "Observer camera OFF." << endl;
    } else {
        use_camera(kObserverCameraName());
        cout << "Observer camera ON." << endl;
    }
}

void Runtime::use_camera(const string& camera_name)
{
    if (_cameras.count(camera_name) < 1) {
        cerr << "Could not find render camera" << camera_name << endl;
        return;
    }

    _render_camera = _cameras[camera_name];

    //Unless we use our debug observer camera, cull camera equals
    //render camera.
    if (camera_name != kObserverCameraName())
        _cull_camera = _render_camera;
}

void Runtime::insert_node(const rtr_format::TransformNode& node)
{
    const string& id = node.id();
    const TransformNode* dependency = NULL;

    if (_node_map.count(node.dependency()) > 0) {
        dependency = _node_map[node.dependency()].get();
    }

    TransformNodeRef new_node(new TransformNode(id, dependency));

    for (int i = 0; i < node.transform_size(); ++i) {
        const rtr_format::Transform& t = node.transform(i);

        if (t.type() == rtr_format::Transform::TRANSLATE) {
            const rtr_format::Transform_Translate& v = t.translate();

            vec3 value(v.value().x(), v.value().y(), v.value().z());
            new_node->add_translate(t.id(), value, _evaluator);
        } else if (t.type() == rtr_format::Transform::ROTATE) {
            const rtr_format::Transform_Rotate& v = t.rotate();

            vec3 axis(v.axis().x(), v.axis().y(), v.axis().z());
            new_node->add_rotate(t.id(), axis, v.angle(), _evaluator);
        } else if (t.type() == rtr_format::Transform::SCALE) {
            const rtr_format::Transform_Scale& v = t.scale();

            vec3 value(v.value().x(), v.value().y(), v.value().z());
            new_node->add_scale(t.id(), value, _evaluator);
        } else if (t.type() == rtr_format::Transform::MATRIX) {
            const rtr_format::Mat4f& v = t.matrix();

            mat4 matrix(v.m00(),v.m10(), v.m20(), v.m30(),
                        v.m01(),v.m11(), v.m21(), v.m31(),
                        v.m02(),v.m12(), v.m22(), v.m32(),
                        v.m03(),v.m13(), v.m23(), v.m33());
            new_node->add_matrix_transform(t.id(), matrix, _evaluator);
        } else if (t.type() == rtr_format::Transform::LOOKAT) {
            const rtr_format::Transform_LookAt& v = t.lookat();

            vec3 position(v.position().x(), v.position().y(), v.position().z());
            vec3 focus(v.point_of_interest().x(), 
                       v.point_of_interest().y(),
                       v.point_of_interest().z());
            vec3 up(v.up().x(), v.up().y(), v.up().z());

            new_node->add_lookat(t.id(), position, focus, up, _evaluator);
        } else if (t.type() == rtr_format::Transform::SKEW) {
            // TODO: Add this feature
        }
    }

    //after adding all transformations, we want to update the node
    //in order to set its matrix cache
    new_node->update();

    if (_node_map.count(id) > 0) {
        cout << "Warning: Scene already contains a TransformNode with ID " 
             << id << ". Skipping TransformNode." << endl;
        return;
    }
    _node_map[id] = new_node;
    _nodes.push_back(new_node);
}

void Runtime::insert_light(const rtr_format::Light& light)
{
    const string& id = light.id();
    const string& node_id = light.transform_node();
    vec3 intensity(light.intensity().x(), 
                   light.intensity().y(), 
                   light.intensity().z());
    float multiplier = light.multiplier();
    Light::Type type;
    const TransformNodeRef& node = _node_map[node_id];

    switch(light.type()) {
    case rtr_format::Light::DIRECTIONAL:
        type = Light::DIRECTIONAL; break;
    case rtr_format::Light::POINT:
        type = Light::POINT; break;
    case rtr_format::Light::SPOT:
        type = Light::SPOT; break;
    default:
        assert(0);
    }

    LightRef light_ref(new Light(id, 
                                 type, 
                                 intensity, 
                                 light.use_shadowmaps(),
                                 multiplier, 
                                 _evaluator, 
                                 node));

    vec4 attenuation(0);

    if (light.has_near_attenuation()) {
        attenuation.x = light.near_attenuation().start();
        attenuation.y = light.near_attenuation().end();
    }

    if (light.has_far_attenuation()) {
        attenuation.z = light.far_attenuation().start();
        attenuation.w = light.far_attenuation().end();
    }

    if (light.type() == rtr_format::Light::POINT) {     
        light_ref->set_attenuation(attenuation);
        
    } else if (light.type() == rtr_format::Light::SPOT) {
        light_ref->set_attenuation(attenuation);

        vec2 spot_attenuation(light.spot_attenuation().hotspot_angle(),
                              light.spot_attenuation().falloff_angle());

        light_ref->set_spot_attenuation(spot_attenuation);
                                            
        if (light_ref->use_shadowmaps()) {
            light_ref->set_shadowmap_id(_shadowmap_count);
            ++_shadowmap_count;
        } else {
        }
    }

    if (_lights.count(id) > 0) {
        cout << "Warning: Scene already contains a light with ID " 
             << id << ". Skipping light." << endl;
        return;
    }

    _lights[id] = light_ref;
}

void Runtime::insert_camera(const rtr_format::Camera& camera)
{
    const string& id = camera.id();
    const string& node_id = camera.transform_node();
    const TransformNodeRef& node = _node_map[node_id];

    if (_cameras.count(id) > 0) {
        cout << "Warning: Scene already contains a camera with ID " 
             << id << ". Skipping camera." << endl;
        return;
    }

    Camera::FOV_AXIS fov_axis = Camera::Y_AXIS;
    if (camera.fov_axis() == rtr_format::Camera::X_AXIS)
        fov_axis = Camera::X_AXIS;

    _cameras[id] = CameraRef(new Camera(id, 
                                        camera.fov_angle(),
                                        fov_axis,
                                        camera.z_near(), camera.z_far(),
                                        _evaluator, node));

    if (camera.has_target_node()) {
        if (_node_map.count(camera.target_node()) > 0)
            _cameras[id]->set_target(_node_map.at(camera.target_node()));
        else
            cout << "Could not find camtargetnode'" << camera.target_node() 
                 << "'." << endl;
    }
}

void Runtime::insert_geometry(const rtr_format::Geometry& geometry)
{
    const string& id = geometry.id();
    const string& node_id = geometry.transform_node();
    const string& mesh_id = geometry.mesh_id();
    const string& material_id = geometry.material_id();

    TransformNodeRef node = _node_map[node_id];
    
    shared_ptr<rtr_format::Material> material;
    _db_loader->read(material_id, material);

    MaterialInstanceRef material_instance;

    if (!material) {
        cout << "Could not load material " << material_id << "." << endl;
        material_instance = _material_manager.error_material(material_id);
    } else {
        material_instance = _material_manager.get_instance(*material);
    }

    GPUMeshRef mesh = get_mesh(mesh_id);

    GeometryRef geo(new Geometry(id, mesh, node, material_instance, material_id));

    //We have to be a little more careful about housekeeping our _geometries 
    //When overwriting an existing entry in _geometries, pointers in the Octree
    //will be invalidated and we encounter random crashes in other places.
    //Therefore we double-check the ID of a geometry if it is already present
    if (_geometries.count(id) > 0) {
        cout << "Warning: Scene already contains a geometry with ID " 
             << id << ". Skipping this geometry." << endl;
        return;
    }
    _geometries[id] = geo;
}

GPUMeshRef Runtime::get_mesh(const string& mesh_id)
{
    if(_meshes.count(mesh_id) > 0) {
        return _meshes[mesh_id];
    }

    shared_ptr<rtr_format::Mesh> mesh;
    _db_loader->read(mesh_id, mesh);

    if (!mesh) {
        cout << "Could not load mesh." << endl;
    }

    GLenum primitive_type;

    switch(mesh->primitive_type()) {
    case rtr_format::Mesh::TRIANGLES:
        primitive_type = GL_TRIANGLES; break;
    case rtr_format::Mesh::TRIANGLE_STRIP:
        primitive_type = GL_TRIANGLE_STRIP; break;
    }

    //iterator over layer sources, if we haven't initialize a GPU layer source
    //yet, add it to the cache.
    for (int i = 0; i < mesh->layer_size(); ++i) {
        const string& source_id = mesh->layer(i).source();
        boost::shared_ptr<rtr_format::LayerSource> layer;
        _db_loader->read(source_id, layer);

        LayerSourceInitializer source(*layer);
        
        if (_sources.count(source_id) < 1) {
            _sources[source_id] = 
                GPULayerSourceRef(new GPULayerSource(source));
        }
    }

    MeshInitializer initializer(*mesh);

    GPUMeshRef gpu_mesh(new GPUMesh(initializer, _sources));
    _meshes[mesh_id] = gpu_mesh;

    return gpu_mesh;
}

void Runtime::start_animation(const string& animation_id, float offset)
{
    shared_ptr<rtr_format::Animation > anim;
    _db_loader->read(animation_id, anim);
    if (_animations_holder.count(anim->id()) > 0) {
        cerr << "Animation with ID " << anim->id() << " already exists."
             << "Your rtr file might be corrupt. Skipping animation." << endl;
        return;
    }
    _animations_holder[anim->id()] = anim;
    _evaluator.add_animation(*anim.get(), offset);
}

void Runtime::reload_materials() {
    _material_manager.reload(_db_loader);

    clear_query(_octree_query);
}
                             
void Runtime::update(const Timer& timer)
{
    _evaluator.update_absolute(timer.now());

    for (size_t i = 0; i < _nodes.size(); ++i) {
        _nodes[i]->update();
    }

    if (!_octree->update()) {
        cout << "Octree is too small and will be resized." << endl;
        setup_octree();
    }

    _dust_particles.update(timer.diff(), _render_camera);
}

void Runtime::setup_octree()
{
    Sphere world_sphere;

    //If we previously had an octree, we will use the previous world size as
    //a starting point. Therefore, if animated objects move outside the existing
    //octree, the new octree will also accomodate the size of the previous octree
    if (_octree != NULL) {
        world_sphere = Sphere(_octree->world_size()*0.5f, _octree->center());
    } else if (_geometries.size() > 0) {
        world_sphere = _geometries.begin()->second->bounding_volume().sphere();
    } else {
        cout << "Error: No geometries to insert." << endl;
        return;
    }

    //from the current we get a good estimate for
    //the world size.
    map<string, GeometryRef>::const_iterator it_geo;
    for (it_geo = _geometries.begin(); it_geo != _geometries.end(); ++it_geo)
    {
        world_sphere = Sphere::unite(world_sphere, 
                                    it_geo->second->bounding_volume().sphere());
    }

    float world_size = world_sphere.radius() * 2;
    vec3 world_center = world_sphere.center();

    cout << "World size: " << world_size << endl;
    cout << "World center: " << world_center << endl;

    int octree_depth = config.octree_max_depth();
    bool do_collect_statistics = config.octree_statistics();
    bool do_debug_rendering = config.octree_debug();

    LooseOctree::StorageType octree_storage_type;
    if (config.octree_storage_type() == RtrPlayerConfig::FULL_ARRAY)
        octree_storage_type = LooseOctree::FULL_ARRAY;
    else if (config.octree_storage_type() == RtrPlayerConfig::SPARSE_MAP)
        octree_storage_type = LooseOctree::SPARSE_MAP;

    if (_octree != NULL) {
        delete _octree;
        _octree = NULL;
    }

    _octree = new LooseOctree(world_size, world_center,
                              octree_depth, octree_storage_type,
                              do_collect_statistics,
                              do_debug_rendering);
    
    //finally, insert into the octree
    for (it_geo = _geometries.begin(); it_geo != _geometries.end(); ++it_geo)
    {
        _octree->insert(it_geo->second.get());
    }
}

void Runtime::clear_query(LooseOctree::QueryResult& octree_query) 
{
    if ((int)octree_query.size() != _material_manager.material_count()) {
        octree_query.resize(_material_manager.material_count());
    }

    for (size_t i = 0; i < octree_query.size(); ++i) {
        octree_query[i].clear();
    }
}

void Runtime::draw_with_octree(const Frustum& frustum, int program)
{
    float aspect = _viewport.aspect();

    clear_query(_octree_query);

    _octree->query(frustum, _octree_query);

    TextureArray& shadowmaps = _shadow_fbo->get_texture_array(1);
    shadowmaps.bind();


    for (size_t i = 0; i < _octree_query.size(); ++i) {
        if (_octree_query[i].size() < 1)
            continue;

        Shader& shader = _material_manager.get_shader(program, i);

        shader.bind();
        shader.set_uniform_block("Shared", *_shared_UBO);
        shader.set_uniform("shadowmaps", shadowmaps);

        list<const Geometry*>::const_iterator geo_it;
        for (geo_it = _octree_query[i].begin(); 
             geo_it != _octree_query[i].end(); ++geo_it) {
            const Geometry* geo = *geo_it;

            const MaterialInstanceRef& material = geo->material_instance();

            material->bind(shader);

            setup_transform_uniforms(*_transform_UBO,
                                     geo->get_local_to_world(),
                                     _render_camera->get_world_to_local(),
                                     _render_camera->get_projection_matrix(aspect));
            shader.set_uniform_block("Transform", *_transform_UBO);

            geo->draw(shader);

            material->unbind();
        }

        shader.unbind();
    }

    shadowmaps.unbind();

    //If enabled we draw bounding geometry
    //Note that we rather re-iterate the block as we want to avoid
    //to many shader-switches (which is the whole point of the query
    //structure)
    if (config.draw_bounding_geometry()) {

        _line_shader->bind();
        _line_shader->set_uniform("color", vec4(0, 0, 1, 1)); 

        for (size_t i = 0; i < _octree_query.size(); ++i) {
            
            if (_octree_query[i].size() < 1)
                continue;

            list<const Geometry*>::const_iterator geo_it;
            for (geo_it = _octree_query[i].begin(); 
                 geo_it != _octree_query[i].end(); ++geo_it) {
            
                const Sphere& bounding_sphere = (*geo_it)->bounding_volume().sphere();                

                float aspect = _viewport.aspect();
                const mat4& view_matrix = _render_camera->get_world_to_local();
                mat4 projection_matrix = _render_camera->get_projection_matrix(aspect);

                float scale_factor = bounding_sphere.radius() / glm::sqrt(3.0f);
                mat4 model = 
                    glm::translate(bounding_sphere.center()) * 
                    glm::scale(vec3(scale_factor, scale_factor, scale_factor));

                mat4 model_view_projection = projection_matrix * view_matrix * model;

                _line_shader->set_uniform("model_view_projection", model_view_projection);

                _wired_cube->draw(*_line_shader);

            }
        }

        _line_shader->unbind();
    }
    
    if (config.octree_debug()) {
        draw_debug_info();
    }

    draw_cull_frustum();

}

void Runtime::draw_debug_info()
{
    //If octree debugging is enabled, we will render the bounding boxes as well
    //render debug info of octree
    if (_octree->has_debug_info() && !config.use_depth_of_field()) {

        float aspect = _viewport.aspect();

        const mat4& view_matrix = _render_camera->get_world_to_local();
        mat4 projection_matrix = _render_camera->get_projection_matrix(aspect);

        _line_shader->bind();
        _line_shader->set_uniform("color", vec4(1, 0, 0, 1)); 

        LooseOctree::DebugQueryResult::const_iterator it_entry;
        for ( it_entry = _octree->debug_info().begin(); 
              it_entry !=_octree->debug_info().end(); 
              ++it_entry ) {

            //translate to center
            float scale_factor = glm::length(it_entry->half_diagonal()) / glm::sqrt(3.0f);
            mat4 model = glm::translate(it_entry->center()) * glm::scale(vec3(scale_factor, scale_factor, scale_factor));

            mat4 model_view_projection = projection_matrix * view_matrix * model;

            _line_shader->set_uniform("model_view_projection", model_view_projection);

            _wired_cube->draw(*_line_shader);

        }

        _line_shader->unbind();
    }
}

void Runtime::draw_cull_frustum() {
    //We draw the cull frustum, if the render camera is not the cull camera
    //(e.g. in observer mode)
    if ( (_render_camera != _cull_camera) && !config.use_depth_of_field()) {

        glDisable(GL_DEPTH_TEST);

        float aspect = _viewport.aspect();

        const mat4& render_view_matrix = 
            _render_camera->get_world_to_local();

        mat4 render_projection_matrix = 
            _render_camera->get_projection_matrix(aspect);

        mat4 cull_projection_matrix = 
            _cull_camera->get_projection_matrix(aspect);

        mat4 cull_projection_matrix_inv = 
            glm::inverse(cull_projection_matrix);

        mat4 model = 
            _cull_camera->get_local_to_world() * cull_projection_matrix_inv;

        mat4 model_view_projection = 
            render_projection_matrix * render_view_matrix * model;

        _line_shader->bind();
        _line_shader->set_uniform("color", vec4(0, 1, 0, 1));

        _line_shader->set_uniform("model_view_projection", model_view_projection);

        _wired_cube->draw(*_line_shader);

        _line_shader->unbind();

        glEnable(GL_DEPTH_TEST);
    }
}

void Runtime::draw_directly(int program)
{
    float aspect = _viewport.aspect();
    
    TextureArray& shadowmaps = _shadow_fbo->get_texture_array(1);

    shadowmaps.bind();

    for(map<string, GeometryRef>::iterator i = _geometries.begin();
        i != _geometries.end(); ++i) {
        GeometryRef& geo = i->second;
        const MaterialInstanceRef& material = geo->material_instance();
        Shader& shader = 
            _material_manager.get_shader(program, material->material_id());

        shader.bind();
        material->bind(shader);
        shader.set_uniform_block("Shared", *_shared_UBO);
        shader.set_uniform("shadowmaps", shadowmaps);

        setup_transform_uniforms(*_transform_UBO,
                                 geo->get_local_to_world(),
                                 _render_camera->get_world_to_local(),
                                 _render_camera->get_projection_matrix(aspect));
        shader.set_uniform_block("Transform", *_transform_UBO);

        geo->draw(shader);

        material->unbind();
        shader.unbind();
    }
    
    shadowmaps.unbind();
}


void Runtime::draw_shadow(mat4 shadow_transform, mat4 shadow_view)
{
    _shadow_shader.bind();

    _shadow_shader.set_uniform("depth_bias", config.shadowmap_bias());

    for(map<string, GeometryRef>::iterator i = _geometries.begin();
        i != _geometries.end(); ++i) {
        GeometryRef& geo = i->second;

        mat4 model = geo->get_local_to_world();

        _shadow_shader.set_uniform("model_view_projection", 
                                   shadow_transform * model);
        _shadow_shader.set_uniform("model_view", 
                                   shadow_view * model);
        geo->draw(_shadow_shader);
    }

    _shadow_shader.unbind();
}

void Runtime::draw_shadow_with_octree(const Frustum& frustum, 
                                      mat4 shadow_transform,
                                      mat4 shadow_view)
{
    clear_query(_octree_query);

    _octree->query(frustum, _octree_query);
    
    _shadow_shader.bind();

    _shadow_shader.set_uniform("depth_bias", config.shadowmap_bias());
    for (size_t i = 0; i < _octree_query.size(); ++i) {
        list<const Geometry*>::const_iterator geo_it;
        for (geo_it = _octree_query[i].begin(); 
             geo_it != _octree_query[i].end(); ++geo_it) {

            mat4 model = (*geo_it)->get_local_to_world();

            _shadow_shader.set_uniform("model_view_projection", 
                                       shadow_transform * model);
            _shadow_shader.set_uniform("model_view", 
                                       shadow_view * model);
            (*geo_it)->draw(_shadow_shader);
        }
    }

    _shadow_shader.unbind();
}

void Runtime::prepare_shadowmaps()
{
    glEnable(GL_DEPTH_TEST);
    
    _shadowmap_matrices.resize(_shadowmap_count);

    _shadow_fbo->bind();

    mat4 tex(0.5, 0.0, 0.0, 0.0,
             0.0, 0.5, 0.0, 0.0,
             0.0, 0.0, 1.0, 0.0,
             0.5, 0.5, 0.0, 1.0);

    //float sm_far = config.shadowmap_far();                       

    for (map<string, LightRef>::iterator i = _lights.begin();
         i != _lights.end(); ++i) {
        LightRef light = i->second;

        if ( !light->use_shadowmaps() ||
             (light->get_type() != Light::SPOT) )
            continue;

        _shadow_fbo->set_array_index(1, light->shadowmap_id());

        glClearColor(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN(), 
                     0, 
                     0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle_factor = config.shadowmap_spot_angle_factor();
        float fov_angle = float(light->spot_attenuation().y);

        //Note: the bakery usually ensures that a spot lights that uses
        //dynamic shadow maps, has to set near/far attenuation values 
        //properly. Of course that does not mean it could not contain
        //useless values anyway.

        vec4 att = light->attenuation();

        float near_att = light->attenuation().x;
        if (near_att <= 0) {
            near_att = config.shadowmap_near();
        }

        float far_att = light->attenuation().w;
        if (far_att <= 0) {
            far_att = config.shadowmap_far();
        }

        mat4 view = light->get_world_to_local();
        mat4 projection = glm::perspective(angle_factor * fov_angle, 
                                           1.0f, near_att, far_att);

        mat4 vp = projection * view;
        
        Frustum frustum(projection * view);

        if (config.enable_octree_culling()) {
            draw_shadow_with_octree(frustum,  vp, view);
        } else {
            draw_shadow(vp, view);
        }

        _shadowmap_matrices[light->shadowmap_id()] = tex * vp;
        
    }

    _shadow_fbo->unbind();

    TextureArray& shadowmaps = _shadow_fbo->get_texture_array(1);

    if (_shadow_blur != NULL) {
        _shadow_blur->blur_texture_array(*_shadow_fbo, 1);
    }

    shadowmaps.bind();
    shadowmaps.generate_mipmaps();
    shadowmaps.unbind();
}

void Runtime::draw()
{
    float aspect = _viewport.aspect();

    if (config.use_shadowmaps() && _shadowmap_count > 0) {
        prepare_shadowmaps();
    }

    if (config.draw_wireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    _fbo->bind();

    glEnable(GL_DEPTH_TEST);
    glClearColor(config.clear_color().r,
                 config.clear_color().g,
                 config.clear_color().b,
                 _render_camera->get_z_far());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setup_shared_uniforms();
    _shared_UBO->bind();
    _transform_UBO->bind();

    Frustum cull_frustum = _cull_camera->get_frustum(aspect);

    if (config.enable_octree_culling()) {
        draw_with_octree(cull_frustum, _standard_program);
    } else {
        draw_directly(_standard_program);
    }
    
    _transform_UBO->unbind();
    _shared_UBO->unbind();

    mat4 view =  _render_camera->get_world_to_local();
    mat4 projection = _render_camera->get_projection_matrix(aspect);
    mat4 vp = projection * view;

    if (config.dust_debug()) {
        mat4 model = glm::translate(_dust_particles.center());
        model *= glm::scale(_dust_particles.half_size());

        mat4 mvp = vp * model;

        _line_shader->bind();
        _line_shader->set_uniform("color", vec4(1,1,0,1));

        _line_shader->set_uniform("model_view_projection", mvp);

        _wired_cube->draw(*_line_shader);

        _line_shader->unbind();
    }

    _fbo->unbind();

    if (config.draw_wireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    _fbo->blit(1, _rgbz_buffer,
              ivec2(0,0), _viewport.render_size(),
              ivec2(0,0), _viewport.render_size(),
              GL_NEAREST);

    _dust_particles.render(_rgbz_buffer, *_shared_UBO, 
                           _shadow_fbo->get_texture_array(1),
                           vp);

    // Apply post-process effects
    _post_process->apply(_viewport, _rgbz_buffer, *_render_camera, 
                         _dust_particles.get_particle_layer());

}

#define MAX_LIGHT_COUNT 12

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2
#define SHADOWED_SPOT_LIGHT 3

void Runtime::setup_shared_uniforms()
{
    int light_count = 0;
    for (map<string, LightRef>::iterator i = _lights.begin();
         i != _lights.end(); ++i) {
        LightRef light = i->second;

        if (light_count + 1 == MAX_LIGHT_COUNT) {
            cerr << "Too many light sources!!" << endl;
            break;
        }

        if (light->get_type() == Light::POINT) {

            _shared_UBO->set("light_positions", light_count, 
                             light->world_position());
            _shared_UBO->set("light_intensities", light_count,
                             light->calc_multiplied_intensity());
            _shared_UBO->set("light_attenuations", light_count,
                             light->attenuation());
            _shared_UBO->set("light_types", light_count, POINT_LIGHT);
                        
        } else if (light->get_type() == Light::DIRECTIONAL) {

            _shared_UBO->set("light_directions", light_count, 
                             light->world_direction());
            _shared_UBO->set("light_intensities", light_count, 
                             light->calc_multiplied_intensity());
            _shared_UBO->set("light_types", light_count, DIRECTIONAL_LIGHT);
                    
        } else if (light->get_type() == Light::SPOT && 
                   light->use_shadowmaps() &&
                   config.use_shadowmaps() ) 
        {

            vec2 spot_att(cosf(light->spot_attenuation().x * M_PI/180.0 * 0.5),
                          cosf(light->spot_attenuation().y * M_PI/180.0 * 0.5));

            int id = light->shadowmap_id();

            _shared_UBO->set("light_positions", light_count, 
                             light->world_position());
            _shared_UBO->set("light_directions", light_count, 
                             light->world_direction());
            _shared_UBO->set("light_intensities", light_count, 
                             light->calc_multiplied_intensity());
            _shared_UBO->set("light_attenuations", light_count,
                             light->attenuation());
            _shared_UBO->set("light_spot_attenuations", light_count, spot_att);
            _shared_UBO->set("light_shadow_ids", light_count, 
                              id);
            _shared_UBO->set("light_types", light_count, SHADOWED_SPOT_LIGHT);

            //This is a workaround for Catalyst 11.1, where uploading mat4
            //arrays had a bug. Therefeore, until this bug is fixed, we upload
            //four vec4's instead of one mat4.
            _shared_UBO->set("shadow_matrices", id*4, _shadowmap_matrices[id][0]);
            _shared_UBO->set("shadow_matrices", id*4+1, _shadowmap_matrices[id][1]);
            _shared_UBO->set("shadow_matrices", id*4+2, _shadowmap_matrices[id][2]);
            _shared_UBO->set("shadow_matrices", id*4+3, _shadowmap_matrices[id][3]);

        } else if (light->get_type() == Light::SPOT) {

            vec2 spot_att(cosf(light->spot_attenuation().x * M_PI/180.0 * 0.5),
                          cosf(light->spot_attenuation().y * M_PI/180.0 * 0.5));

            _shared_UBO->set("light_positions", light_count, 
                             light->world_position());
            _shared_UBO->set("light_directions", light_count, 
                             light->world_direction());
            _shared_UBO->set("light_intensities", light_count, 
                             light->calc_multiplied_intensity());
            _shared_UBO->set("light_attenuations", light_count,
                             light->attenuation());
            _shared_UBO->set("light_spot_attenuations", light_count, spot_att);
            _shared_UBO->set("light_types", light_count, SPOT_LIGHT);
        }
        
        ++light_count;  
    }

    _shared_UBO->set("shadowmap_min_variance", config.shadowmap_min_variance());
    _shared_UBO->set("shadow_bleed_bias", config.shadowmap_bleed_bias());

    _shared_UBO->set("light_count", light_count);
    _shared_UBO->set("ambient", vec3(0,0,0));
    _shared_UBO->set("camera_world_position",
                     _render_camera->get_world_location());
    _shared_UBO->send_to_GPU();
}

void Runtime::set_depth_of_field(bool enable)
{
    delete _post_process;
    if (enable) {
        _post_process = new DepthOfFieldPostProcess(_viewport);
    } else {
        _post_process = new SimplePostProcess(_viewport);
    }
}

void Runtime::setup_transform_uniforms(UniformBuffer& transform,
                                       const mat4& model,
                                       const mat4& view,
                                       const mat4& projection)
{
    mat3 normal_matrix = mat3(glm::transpose(glm::inverse(model)));
    mat4 model_view_projection = projection * view * model;

    transform.set("model", model);
    transform.set("model_view_projection", model_view_projection);
    transform.set("normal_matrix", normal_matrix);
    transform.send_to_GPU();
}
