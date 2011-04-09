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

#include "VisualSceneProcessor.h"
#include "Baker.h"
#include "BakerCache.h"

#include "Utils.h"

#include "COLLADAFWVisualScene.h"

#include "COLLADAFWTransformation.h"
#include "COLLADAFWLookat.h"
#include "COLLADAFWTranslate.h"
#include "COLLADAFWRotate.h"
#include "COLLADAFWSkew.h"
#include "COLLADAFWMatrix.h"
#include "COLLADAFWScale.h"

#include <sstream>
#include <boost/regex.hpp>
#include <algorithm>

#include "rtr_format.pb.h"

#include "common_const.h"

using namespace ColladaBakery;

VisualSceneProcessor::VisualSceneProcessor(Baker* baker) : 
Processor(baker)
{
}

bool VisualSceneProcessor::process(const CF::Object* c_obj) {
    
    //register ourselves for post_process after in-place processing.
    //Note that the scene post processor must be one of the last post processors
    //to run
    _baker->register_for_postprocess(shared_from_this(), 9999);

    const CF::VisualScene* c_vs = 
                            static_cast<const CF::VisualScene*>(c_obj);

    _scene_id = _baker->get_id(c_vs);
   
    _c_scene_id = c_vs->getUniqueId();

    _rtr_scene.set_name(_scene_id);

    try {
        //TODO: eventually, you could add a rotation in here for swapping
        //      up-axes
        for (size_t i = 0 ; i < c_vs->getRootNodes().getCount(); ++i) {
            process_node(c_vs->getRootNodes()[i], "");
        }

    } catch (std::runtime_error& e) {
        cerr << "Importing transformation: " << e.what() << endl;
        return false;
    }

    return true;
}

void VisualSceneProcessor::process_node( const CF::Node* c_node, 
                                         const string& dependency ) {

    if (c_node->getType() != CF::Node::NODE)
        return;

    string c_node_id = _baker->get_id(c_node);

    //we have to replace any "." in the ID, this might confuse our
    //rtr addressing scheme where "." is only used for subscript access
    //to transforms (e.g. translate.X)
    const boost::regex rx("\\.");
    string replacement("_");
    c_node_id = boost::regex_replace(c_node_id, rx, replacement);

    //create the actual target we are converting to
    rtr_format::TransformNode* rtr_node = _rtr_scene.add_node();
    rtr_node->set_id(c_node_id);
    if (!dependency.empty())
        rtr_node->set_dependency(dependency);

    string transform_dependency = dependency;

    //cache the name of the nodes
    if (!_nodes_name_cache.insert(c_node_id).second) {
        cout << "Error: could not insert into node name cache." << endl;
        _baker->fail();
        return;
    }

    //Process transforms
    for ( size_t iTrafo = 0; 
          iTrafo < c_node->getTransformations().getCount(); 
          ++iTrafo )
    {
        process_trafo(c_node->getTransformations()[iTrafo], rtr_node);
    }

    // Process camera, geometry, light and node instances (will be deferred to
    // post processing).

    for ( size_t iCam = 0; 
          iCam < c_node->getInstanceCameras().getCount(); 
          ++iCam )
    {
        const CF::InstanceCamera* c_inst = 
                                             c_node->getInstanceCameras()[iCam];
        //memorize this, we will resolve this during post-processing
        ResolveData instantiation;
        instantiation.c_node_id = c_node_id;
        instantiation.referenced_id = c_inst->getInstanciatedObjectId();
        instantiation.rtr_node = c_node_id;
        _camera_instances.push_back(instantiation);
    }

    for ( size_t iLight = 0; 
          iLight < c_node->getInstanceLights().getCount(); 
          ++iLight )
    {
        const CF::InstanceLight* c_inst = c_node->getInstanceLights()[iLight];
        //memorize this, we will resolve this during post-processing
        ResolveData instantiation;
        instantiation.c_node_id = c_node_id;
        instantiation.referenced_id = c_inst->getInstanciatedObjectId();
        instantiation.rtr_node = c_node_id;
        _light_instances.push_back(instantiation);
    }

    for ( size_t iGeo = 0; 
          iGeo < c_node->getInstanceGeometries().getCount(); 
          ++iGeo )
    {
        const CF::InstanceGeometry* c_inst = 
                                         c_node->getInstanceGeometries()[iGeo];

        //memorize this, we will resolve this during post-processing

        ResolveData instantiation;
        instantiation.c_node_id = c_node_id;
        instantiation.referenced_id = c_inst->getInstanciatedObjectId();
        instantiation.rtr_node = c_node_id;
        GeometryResolveData geo_instantiation;
        geo_instantiation.resolve_data = instantiation;

        //We add the material binding information to the resolve data
        //we will need this later to connect the processed material
        //with the correct mesh
        //We will also add the COLLADAFW::UniqueId of the instantiated material
        //to our cache in order to process only those material in their respective
        //processor which are actually referenced.
        for (size_t iMat = 0;
             iMat < c_inst->getMaterialBindings().getCount();
             ++iMat)
        {
            const CF::MaterialBinding& c_mb = 
                c_inst->getMaterialBindings()[iMat];

            BakerCache::TexCoordBindingList tex_coord_bdg_cache;
            for (size_t iT = 0; 
                 iT < c_mb.getTextureCoordinateBindingArray().getCount(); 
                 ++iT)
            {
                tex_coord_bdg_cache.push_back(c_mb.getTextureCoordinateBindingArray()[iT]);
            }

            BakerCache::GeomMatIdPair key(c_inst->getInstanciatedObjectId(), 
                                          c_mb.getMaterialId());

            BakerCache::VisualSceneMaterialBinding binding_cache;
            binding_cache.tex_coord_binding_list = tex_coord_bdg_cache;
            binding_cache.referenced_material = c_mb.getReferencedMaterial();

            BakerCache::SceneMaterialBindingMap::value_type val(key, 
                                                                binding_cache);

            std::pair<BakerCache::SceneMaterialBindingMap::iterator, bool>
                insertion = _baker->cache().scene_material_cache.insert(val);
            if (!insertion.second) {

                bool are_equal = std::equal(tex_coord_bdg_cache.begin(), 
                                            tex_coord_bdg_cache.end(), 
                                            insertion.first->second.tex_coord_binding_list.begin(),
                                            c_tex_coord_bgd_predicate);

                are_equal &= ( insertion.first->second.referenced_material == 
                               binding_cache.referenced_material );

                if (!are_equal) {
                    cout << "Warning: multiple UV set bindings for the same "
                         << "material/mesh combination." << endl;
                }
            }

            MatBindingMap::value_type v(c_mb.getMaterialId(), c_mb);
            if (!geo_instantiation.material_bindings.insert(v).second) {
                cout << "Could not insert material binding " << c_mb.getName()
                     << ". This might be the result of a duplicate "
                     << "instance_material binding." << endl;
            }

            _baker->cache().used_materials.insert(c_mb.getReferencedMaterial());
        }

        _geometry_instances.push_back(geo_instantiation);
    }

    for ( size_t iNodeInst = 0; 
          iNodeInst < c_node->getInstanceNodes().getCount(); 
          ++iNodeInst )
    {
        const CF::InstanceNode* c_inst = 
                                          c_node->getInstanceNodes()[iNodeInst];
        //memorize this, we will resolve this during post-processing
        ResolveData instantiation;
        instantiation.c_node_id = c_node_id;
        instantiation.referenced_id = c_inst->getInstanciatedObjectId();
        instantiation.rtr_node = c_node_id;

        NodeResolveData n_data;
        n_data.resolve_data = instantiation;
        //we need to memorize the trafo-dependency where the node was instanced
        n_data.rtr_dependency = dependency;
        _node_instances.push_back(n_data);

        //TODO: re need to dereference these instantiations. We can either
        //cache the original COLLADA data, and parse during postprocess again,
        //or we need to keep lists for each node and the resulted conversions
        //and reset the depdenncy (more efficient). However, this feature
        //is not a top-priority and therefore skipped, at the moment
         cout << "Warning: instance_node is not supported. The instantiation to"
              << " " << c_inst->getInstanciatedObjectId().toAscii() << " will " 
              << "not be  resolved." << endl;
    }

    //call recursively
    for (size_t i = 0 ; i < c_node->getChildNodes().getCount(); ++i) {
        process_node(c_node->getChildNodes()[i], c_node_id);
    }

}

void VisualSceneProcessor::process_trafo(
                                    const CF::Transformation* c_trafo, 
                                    rtr_format::TransformNode* t_node )
{

    //create a new target transform 
    rtr_format::Transform* rtr_trafo = t_node->add_transform();

    string id = "";

    if (c_trafo->getTransformationType() == CF::Transformation::LOOKAT) {
                
        const CF::Lookat* c_la = 
                              static_cast<const CF::Lookat* >( c_trafo );

        //create the name
        id = t_node->id() + "/LookAt";

        rtr_trafo->set_type(rtr_format::Transform::LOOKAT);

        rtr_format::Transform_LookAt* t_look = rtr_trafo->mutable_lookat();

        const CB::Math::Vector3& v_p = c_la->getEyePosition();
        t_look->mutable_position()->set_x(static_cast<float>(v_p[0]));
        t_look->mutable_position()->set_y(static_cast<float>(v_p[1]));
        t_look->mutable_position()->set_z(static_cast<float>(v_p[2]));

        const CB::Math::Vector3& v_i = c_la->getInterestPointPosition();
        t_look->mutable_point_of_interest()->set_x(static_cast<float>(v_i[0]));
        t_look->mutable_point_of_interest()->set_y(static_cast<float>(v_i[1]));
        t_look->mutable_point_of_interest()->set_z(static_cast<float>(v_i[2]));

        const CB::Math::Vector3& v_u = c_la->getUpAxisDirection();
        t_look->mutable_up()->set_x(static_cast<float>(v_u[0]));
        t_look->mutable_up()->set_y(static_cast<float>(v_u[1]));
        t_look->mutable_up()->set_z(static_cast<float>(v_u[2]));
                
    } else if ( c_trafo->getTransformationType() == 
                CF::Transformation::MATRIX) {
            
        const CF::Matrix* c_matrix = 
                              static_cast<const CF::Matrix* >( c_trafo );
        
         //create the name
        id = t_node->id() + "/Matrix";

        rtr_trafo->set_type(rtr_format::Transform::MATRIX);

        rtr_format::Mat4f* t_mat4f = rtr_trafo->mutable_matrix();

        t_mat4f->set_m00(static_cast<float>( c_matrix->getMatrix()[0][0] ));
        t_mat4f->set_m01(static_cast<float>( c_matrix->getMatrix()[0][1] ));
        t_mat4f->set_m02(static_cast<float>( c_matrix->getMatrix()[0][2] ));
        t_mat4f->set_m03(static_cast<float>( c_matrix->getMatrix()[0][3] ));

        t_mat4f->set_m10(static_cast<float>( c_matrix->getMatrix()[1][0] ));
        t_mat4f->set_m11(static_cast<float>( c_matrix->getMatrix()[1][1] ));
        t_mat4f->set_m12(static_cast<float>( c_matrix->getMatrix()[1][2] ));
        t_mat4f->set_m13(static_cast<float>( c_matrix->getMatrix()[1][3] ));

        t_mat4f->set_m20(static_cast<float>( c_matrix->getMatrix()[2][0] ));
        t_mat4f->set_m21(static_cast<float>( c_matrix->getMatrix()[2][1] ));
        t_mat4f->set_m22(static_cast<float>( c_matrix->getMatrix()[2][2] ));
        t_mat4f->set_m23(static_cast<float>( c_matrix->getMatrix()[2][3] ));

        t_mat4f->set_m30(static_cast<float>( c_matrix->getMatrix()[3][0] ));
        t_mat4f->set_m31(static_cast<float>( c_matrix->getMatrix()[3][1] ));
        t_mat4f->set_m32(static_cast<float>( c_matrix->getMatrix()[3][2] ));
        t_mat4f->set_m33(static_cast<float>( c_matrix->getMatrix()[3][3] ));

    } else if ( c_trafo->getTransformationType() == 
                CF::Transformation::ROTATE ) {
            
        const CF::Rotate* c_rot = 
                              static_cast<const CF::Rotate* >( c_trafo );				
            
        id = t_node->id() + "/Rotate";

        rtr_trafo->set_type(rtr_format::Transform::ROTATE);

        rtr_format::Transform_Rotate* t_rot = rtr_trafo->mutable_rotate();

        const CB::Math::Vector3& v_ax = c_rot->getRotationAxis();

        //Catch degenerated case
        if ( (v_ax[0] == 0) && 
             (v_ax[1] == 0) && 
             (v_ax[2] == 0) )
        {
            cout << "Error: Transform Rotate in '" << t_node->id()
                 << "contains a rotation axis of (0, 0, 0). Falling back to"
                 << " (1, 0, 0) with an angle of zero." << endl;

            t_rot->mutable_axis()->set_x(1);
            t_rot->mutable_axis()->set_y(0);
            t_rot->mutable_axis()->set_z(0);
            t_rot->set_angle(0);

        } else {

            t_rot->mutable_axis()->set_x(static_cast<float>(v_ax[0]));
            t_rot->mutable_axis()->set_y(static_cast<float>(v_ax[1]));
            t_rot->mutable_axis()->set_z(static_cast<float>(v_ax[2]));
            t_rot->set_angle(static_cast<float>(c_rot->getRotationAngle()));

        }

    } else if ( c_trafo->getTransformationType() == 
                CF::Transformation::SCALE ) {
            
        const CF::Scale* c_scale = 
                               static_cast<const CF::Scale* >( c_trafo );				
            
        id = t_node->id() + "/Scale";

        rtr_trafo->set_type(rtr_format::Transform::SCALE);

        rtr_format::Transform_Scale* t_scale = rtr_trafo->mutable_scale();

        const CB::Math::Vector3& v = c_scale->getScale();

        t_scale->mutable_value()->set_x(static_cast<float>(v[0]));
        t_scale->mutable_value()->set_y(static_cast<float>(v[1]));
        t_scale->mutable_value()->set_z(static_cast<float>(v[2]));

        //Issue warning when we encounter a non-uniform scale operation
        if (! (v[0] == v[1]) &&
              (v[1] == v[2]) )
        {
            cout << "Warning: Node '" << t_node->id() << "' contains a "
                 << "non-uniform scale transform: "
                 << "(" << v[0] << ", " << v[1] << ", " << v[2] << "). This is"
                 << " currently not "
                 << "supported for "
                 << "geometries. Please make sure that your geometry nodes " 
                 << "do not contain non-uniform scales." << endl;
        }

        if ( (v[0] <= 0) ||
             (v[1] <= 0) ||
             (v[2] <= 0) )
        {
            cout << "Warning: Node '" << t_node->id() << "' contains a "
                 << "scale op that is less or equal than zero. This is very "
                 << "likely causing undesired results. Plase double-check "
                 << "this node. " << endl;
        }
            
    } else if ( c_trafo->getTransformationType() == 
                CF::Transformation::SKEW ) {
            
        const CF::Skew* c_skew = 
                                static_cast<const CF::Skew* >( c_trafo );
            
        id = t_node->id() + "/Skew";

        rtr_trafo->set_type(rtr_format::Transform::SKEW);

        rtr_format::Transform_Skew* t_skew = rtr_trafo->mutable_skew();

        const CB::Math::Vector3& v_rot = c_skew->getRotateAxis();

        t_skew->mutable_rotation_axis()->set_x(static_cast<float>(v_rot[0]));
        t_skew->mutable_rotation_axis()->set_y(static_cast<float>(v_rot[1]));
        t_skew->mutable_rotation_axis()->set_z(static_cast<float>(v_rot[2]));

        const CB::Math::Vector3& v_t = c_skew->getTranslateAxis();

        t_skew->mutable_translation_axis()->set_x(static_cast<float>(v_t[0]));
        t_skew->mutable_translation_axis()->set_y(static_cast<float>(v_t[1]));
        t_skew->mutable_translation_axis()->set_z(static_cast<float>(v_t[2]));

        t_skew->set_angle(static_cast<float>(c_skew->getAngle()));

    } else if ( c_trafo->getTransformationType() == 
                CF::Transformation::TRANSLATE ) {
            
        const CF::Translate* c_translate = 
                           static_cast<const CF::Translate* >( c_trafo );
                
        id = t_node->id() + "/Translate";

        rtr_trafo->set_type(rtr_format::Transform::TRANSLATE);

        rtr_format::Transform_Translate* t_transl = rtr_trafo->mutable_translate();

        const CB::Math::Vector3& v_t = c_translate->getTranslation();

        t_transl->mutable_value()->set_x(static_cast<float>(v_t[0]));
        t_transl->mutable_value()->set_y(static_cast<float>(v_t[1]));
        t_transl->mutable_value()->set_z(static_cast<float>(v_t[2]));
                
    } else {
        throw std::runtime_error("Unknown transformation type.");
    }

    id = make_unique_trafo_name(id);
    rtr_trafo->set_id(id);

    //register with animation requests
    if (c_trafo->getAnimationList().isValid()) {
        BakerCache::AnimListToRTRTargetMap::value_type v(c_trafo->getAnimationList(),
                                                         id);
        _baker->cache().animation_binding_requests.insert(v);
        _baker->cache().animation_used_animlists.push_back(
            c_trafo->getAnimationList());
    }

}

string VisualSceneProcessor::make_unique_trafo_name(const string& pref_id) {
    //TODO: maybe, you should create global unique names instead?
    return Utils::make_unique(pref_id, _trafo_name_cache);
}

bool VisualSceneProcessor::post_process() {

    //Ask the bakery for caches, resolve and add to scene

    //Note: when loading instance_nodes as well in future, we will have to 
    //double-check each objects ID with a cache

    //geometries

    // mesh_path
    //! one instance_geometry leads to several rtr-meshes!
    list<GeometryResolveData>::const_iterator it_geo;
    for ( it_geo = _geometry_instances.begin();
          it_geo != _geometry_instances.end();
          ++it_geo )
    {
        const ResolveData& geo_resolve = it_geo->resolve_data;
        //lookup the bake cache
        BakerCache::GeometryBakeCache::const_iterator it_conv = 
                     _baker->cache().geometries.find(geo_resolve.referenced_id);

        if (it_conv == _baker->cache().geometries.end()) {
            //This might have been a geometry that we don't support yet.
            cout << "Instance to " << geo_resolve.referenced_id.toAscii();
            cout << " could not be resolved." << endl;
            cout << endl;
            continue;
        }

        //add a rtr-geometry in the rtr-scene with the appropriate 
        //transform node
        GeometryProcessor::MeshToMaterialMap::const_iterator it_mmp;
        for ( it_mmp = it_conv->second.rtr_meshes.begin();
              it_mmp != it_conv->second.rtr_meshes.end();
              ++it_mmp )
        {
            rtr_format::Geometry* rtr_geo = _rtr_scene.add_geometry();
            rtr_geo->set_transform_node(geo_resolve.rtr_node);
            string rtr_id = it_geo->resolve_data.c_node_id + "_" 
                            + it_mmp->first + "_instance";
            rtr_geo->set_id( rtr_id );
            cout << "RTR Geo with: " << rtr_id << std::endl;
            rtr_geo->set_mesh_id(it_mmp->first);

            //MaterialId (of mesh) -> ColladaUniqueId -> rtr_material string
            MatBindingMap::const_iterator it_mat = 
                                it_geo->material_bindings.find(it_mmp->second);
            if (it_mat != it_geo->material_bindings.end()) {
                
                CF::UniqueId c_mat_id = it_mat->second.getReferencedMaterial();

                //TODO: we should deal with the texture coordinate binding
                //more carefully in here. At the moment we just make a few
                //assumptions about which texture channels should be used
                //for which types of textures.

                BakerCache::MaterialBakeCache::const_iterator it_baked_mat = 
                                      _baker->cache().materials.find(c_mat_id);

                if (it_baked_mat == _baker->cache().materials.end()) {
                    cout << "Error: Could not resolve material " << 
                        c_mat_id.toAscii() << "." << endl;
                    cout << "Using 'error material' instead." << endl;
                    rtr_geo->set_material_id(rtr::ERROR_MATERIAL_NAME());
                } else {
                    //Found the baked material, set its cached rtr-ID
                    string rtr_mat_id = it_baked_mat->second.rtr_material_id;
                    rtr_geo->set_material_id(rtr_mat_id);
                }

            } else {
                cout << "Warning: Could not resolve material binding '"
                     << it_mmp->second << "' of mesh " << rtr_id << endl;
                cout << "         Setting 'error material' instead." << endl;
                rtr_geo->set_material_id(rtr::ERROR_MATERIAL_NAME());
            }

        }

    }

    //get cameras
    list<ResolveData>::const_iterator it_cam;
    for ( it_cam = _camera_instances.begin();
          it_cam != _camera_instances.end();
          ++it_cam )
    {
        //get the camera
        BakerCache::CameraBakeCache::const_iterator it_conv = 
                            _baker->cache().cameras.find(it_cam->referenced_id);

        if (it_conv == _baker->cache().cameras.end()) {
            cout << "Could not resolve camera instance: " << it_cam->referenced_id.toAscii();
            cout << ". Skipping import." << endl;
            cout << endl;
            continue;
        }

        const rtr_format::Camera * rtr_cache_cam = &(it_conv->second);

        //TODO: if we had multiple instance of the same camera in here, 
        //we would have to take of their ID, in this case the animation
        //ID rtr target in CamerProcessor.cpp would also have to be adapted...
        //This case, however, seems rather rare.

        //finally insert into scene
        rtr_format::Camera * rtr_new_cam = _rtr_scene.add_camera();
        rtr_new_cam->CopyFrom(*rtr_cache_cam);

        //set the transform node
        rtr_new_cam->set_transform_node(it_cam->rtr_node);

        //check if we have a node that defines a target node for this camera.
        //By convention we just check for a node that conforms to
        //a CAMNODENAME_Target naming sceheme.
        //Note: the prefix has to be the node id, not the camera id!
        string target_id = rtr_new_cam->transform_node() + "_Target";
        std::set<string>::const_iterator it = _nodes_name_cache.find(target_id);
        if (it != _nodes_name_cache.end())
            rtr_new_cam->set_target_node(target_id);

    }

    //get lights
    list<ResolveData>::const_iterator it_light;
    for ( it_light = _light_instances.begin();
          it_light != _light_instances.end();
          ++it_light )
    {
        //get the light
        BakerCache::LightBakeCache::const_iterator it_conv = 
                            _baker->cache().lights.find(it_light->referenced_id);

        if (it_conv == _baker->cache().lights.end()) {
            cout << "Could not resolve light instance: " << it_light->referenced_id.toAscii();
            cout << ". Skipping import." << endl;
            cout << endl;
            continue;
        }

        const rtr_format::Light * rtr_cache_light = &(it_conv->second);

        //finally insert into scene
        rtr_format::Light * rtr_new_light = _rtr_scene.add_light();
        rtr_new_light->CopyFrom(*rtr_cache_light);

        //set the transform node
        rtr_new_light->set_transform_node(it_light->rtr_node);
    }

    //as all animation have been processed by now, we can now look up, which
    //rtr animations are active for this scene and store their IDs
   
    BakerCache::UniqueIdList::const_iterator it_anim;
    for ( it_anim = _baker->cache().animation_used_animlists.begin(); 
          it_anim != _baker->cache().animation_used_animlists.end(); 
          ++it_anim ) 
    {
        BakerCache::AnimListToRTRAnimMap::iterator it_rtr_anims = 
                       _baker->cache().animation_binding_results.find(*it_anim);

        if (it_rtr_anims != _baker->cache().animation_binding_results.end()) {
            list<string>::const_iterator rtr_anim;
            for ( rtr_anim = it_rtr_anims->second.begin();
                  rtr_anim != it_rtr_anims->second.end();
                  ++rtr_anim )
            {
                _rtr_scene.add_animation(*rtr_anim);
            }
        }

    }

    //TODO: if you are not careful, a similar named scene (which is not required
    //to be unique, could overwrite another one by accident)
    bool b = _baker->write_baked(_rtr_scene.name(), 
                                 &_rtr_scene);
    if (!b) {
    	cout << "Error, and retruring... " << endl;
        return false;
    }

    //enter this scene in the cache
    BakeCache c;
    c.id = _scene_id;

    BakerCache::VisualSceneBakeCache::value_type v( _c_scene_id, c );
    if (!_baker->cache().scenes.insert(v).second) {
        cout << "Error inserting into bake cache." << endl;
        return false;
    }

    return true;
}
