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

#include "GeometryProcessor.h"
#include "COLLADAFWGeometry.h"
#include "COLLADAFWMesh.h"
#include "COLLADAFWMeshPrimitive.h"
#include "COLLADAFWTriangles.h"
#include "COLLADAFWTristrips.h"
#include "Baker.h"
#include "Utils.h"
#include "rtr_format.pb.h"

#include <limits>

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/scoped_array.hpp>
#include <boost/regex.hpp>

using namespace ColladaBakery;

GeometryProcessor::GeometryProcessor(Baker* baker) :
    Processor(baker), _idx_count(0)
{
}

bool GeometryProcessor::process(const CF::Object* c_obj) {
    const CF::Geometry * c_geo = static_cast<const CF::Geometry*>(c_obj);

    //We will have to register the identifiers of our conversions with the 
    //CF::UniqueId that we get during parsing. This is necessary since
    //we will have to resolve some dependencies (e.g. from instance_geometry
    //to geometry) at a later point

    _c_id = c_geo->getUniqueId();

    if (c_geo->getType() == CF::Geometry::GEO_TYPE_MESH) {

        const CF::Mesh* c_mesh = static_cast<const CF::Mesh*>(c_geo);

        //prepare the arrays to access and read from
        const CF::MeshVertexData& c_pos = c_mesh->getPositions();
        const CF::MeshVertexData& c_nml = c_mesh->getNormals();
        const CF::MeshVertexData& c_clrs = c_mesh->getColors();
        const CF::MeshVertexData& c_uvs = c_mesh->getUVCoords();

        _c_mesh_id = _baker->get_id(c_mesh);

        //Note: Obviously, the CF guarantees, that we can reuse the
        //indices into one datastructure. That also means, we can cache the 
        //n-tuple of the indices and look up our own single index for that.

        //for this whole mesh we have to determine the maximum number of tuples
        //we encounter. We can derive this number by counting the input_infos
        //in the individual data streams (we assume that for positions and
        //normals, there is only one layer)

        //Note that we take into consideration that primitives that index into
        //the per-mesh data streams might not always use all possible vertex 
        //attributes. For example, one primitive might use the positions and 
        //normals layer no vertex colors at all. If a primitive does not use
        //a vertex attribute, we will just nullify these indices in order to
        //retrieve a correct hash map.
        int max_layer_cnt = 1;
        //assert( c_pos.getNumInputInfos() == 1);
        int num_nml_layer_sources = 0;
        if (c_mesh->hasNormals()) {
            num_nml_layer_sources = 1;
            //assert( c_nml.getNumInputInfos() == 1);
        }
        max_layer_cnt += num_nml_layer_sources;
        int num_color_layer_sources = c_clrs.getNumInputInfos();
        max_layer_cnt += num_color_layer_sources;
        int num_uv_layer_sources = c_uvs.getNumInputInfos();
        max_layer_cnt += num_uv_layer_sources;
        
        //In order to re-index from multiple indices to one single index, we
        //assemble a vector of multiple indices, with the following ordering: 
        //POS_IDX, NML_IDX, CLRS_INDICES, UV_INDICES

        std::ostringstream c_mesh_id_ss;
        c_mesh_id_ss << _c_mesh_id;
        //Note: at the moment, we only support triangles
        for ( size_t iPrim = 0; 
              iPrim < c_mesh->getMeshPrimitives().getCount(); 
              ++iPrim) 
        {

            const CF::MeshPrimitive* c_prim = 
                                            c_mesh->getMeshPrimitives()[iPrim];

            //Generate a good name for this primitive
            if (iPrim > 0)
                c_mesh_id_ss << _c_mesh_id << "_" << iPrim;

            //TODO: make this a bit more general, and not only import triangles
            if (c_prim->getPrimitiveType() == CF::MeshPrimitive::TRIANGLES) {

                const CF::Triangles* c_tri = 
                               static_cast<const CF::Triangles*>(c_prim);
               
                //create a new rtr mesh and add it to the info data structure
                MeshInfo rtr_mesh_info;
                rtr_mesh_info.rtr_mesh = RTRMeshRef(new rtr_format::Mesh());

                rtr_mesh_info.rtr_mesh->set_id(c_mesh_id_ss.str());
                rtr_mesh_info.rtr_mesh->set_primitive_type(rtr_format::Mesh::TRIANGLES);

                //TODO: test the following scenario, which might break this 
                //output assemble: some sub-meshes index only into 2nd clr layer
                //but not into first at all...

                //An LayerAssemblyArray is an array of Assemblies, which 
                //represent all necessary information to assemble data streams 
                //from this particular primitive
                LayerAssemblyArray assembly;
                for (int iAttr = 0; iAttr < max_layer_cnt; ++iAttr)
                    assembly.push_back(LayerAssembly()); //initialize as empty

                //Initializing layers for this mesh

                //vertex layer
                LayerAssembly vtx_ass = setup_layer( 
                                            kPositionsLayerName(),
                                            rtr_mesh_info.rtr_mesh.get(),
                                            c_pos,
                                            &c_tri->getPositionIndices() );
                
                assembly[0] = vtx_ass;

                if (c_prim->hasNormalIndices()) {
                    assert(num_nml_layer_sources == 1);
                    //get/initialize the normals layer
                    LayerAssembly nml_ass = setup_layer( 
                                                kNormalsLayerName(),
                                                rtr_mesh_info.rtr_mesh.get(),
                                                c_nml,
                                                &c_tri->getNormalIndices() );
                    assembly[1] = nml_ass;

                }

                //color .. color_n layers

                if (c_prim->hasColorIndices()) {

                    size_t cnt = c_prim->getColorIndicesArray().getCount();
                    assert(cnt == c_clrs.getNumInputInfos());
                    for ( size_t iClr = 0; 
                          iClr < cnt; 
                          ++iClr ) 
                    {
                        const CF::IndexList * c_idx_list = 
                                            c_prim->getColorIndicesArray()[iClr];

                        //TODO-SET: what to do with set_index, get_stride in here
                        //      how is that conflicting/interacting with
                        //      CF::MeshVertexData?

                        //!! I think you should replace iClr here with set_index
                        std::ostringstream oss;
                        oss << kColorsLayerName();
                        if (iClr > 0)
                            oss << "_" << iClr;

                        LayerAssembly clr_ass = setup_layer( 
                                                    oss.str(),
                                                    rtr_mesh_info.rtr_mesh.get(),
                                                    c_clrs,
                                                    &c_idx_list->getIndices(),
                                                    iClr );

                        assembly[num_nml_layer_sources + 1 + iClr] = clr_ass;                      
                    }
                }

                if (c_prim->hasUVCoordIndices()) {

                    size_t cnt = c_prim->getUVCoordIndicesArray().getCount();
                    assert(cnt == c_uvs.getNumInputInfos());
                    for ( size_t iUV = 0; 
                          iUV < cnt; 
                          ++iUV ) 
                    {
                        const CF::IndexList * c_idx_list = 
                                        c_prim->getUVCoordIndicesArray()[iUV];

                        std::ostringstream oss;
                        oss << kUvLayerName();
                        if (iUV > 0)
                            oss << "_" << iUV;

                        string layer_name = oss.str();
                        LayerAssembly uvs_ass = setup_layer( 
                                                    layer_name,
                                                    rtr_mesh_info.rtr_mesh.get(),
                                                    c_uvs,
                                                    &c_idx_list->getIndices(),
                                                    iUV );

                        SetIndexToNameMap::value_type v(c_idx_list->getSetIndex(), 
                                                        layer_name);

                        if (!rtr_mesh_info.uv_set_to_layername.insert(v).second) {
                            cout << "ERROR: Could not insert into cache between" 
                                 << " uv coord set id and local layer name."
                                 << endl;
                            return false;
                        }

                        //add to cache to UV coord determination
                        NameToRTRVALayerMap::value_type val(layer_name, 
                                                          uvs_ass.rtr_layer);
                        if (!rtr_mesh_info.rtr_layer_cache.insert(val).second) {
                            cout << "ERROR: Could not insert into local vertex attribute layer "
                                 << "cache." << endl;
                        }

                        size_t ass_pos = num_nml_layer_sources+1+num_color_layer_sources+iUV;
                        assembly[ass_pos] = uvs_ass;                      
                    }
                }

                MultiIndex c_idx_set(max_layer_cnt);
                //finally, let's iterate over the indices, copy data and re-index
                //where necessary
                for ( size_t iVtx = 0; 
                      iVtx<assembly[0].c_indices->getCount();
                      ++iVtx) 
                {

                    assembly_to_multi_index( assembly, 
                                             c_idx_set,
                                             iVtx );

                    //see if we already have a single-index conversion for this
                    //particular combination of indices (multi-index)

                    IdxLookup::const_iterator it = _idx_cache.find(c_idx_set);
                    if (it != _idx_cache.end()) {
                        //Already have this combination, use old indx
                        rtr_mesh_info.rtr_mesh->add_index_data(it->second);
                    } else {
                        
                        //This combination is new, which means, we have to 
                        //add the data tuple as well, append a new index and
                        //use this index for this particular combination.

                        //let's copy data first
                        LayerAssemblyArray::iterator it; 
                        for (it = assembly.begin(); it != assembly.end(); ++it) 
                        {
                            //only process if this is a valid assembly, if any
                            //member is NULL we should skip this, i.e. that
                            //means a primitive is just not using one the 
                            //available layer source
                            if (it->c_indices == NULL)
                                continue;

                            //TODO: could we pack this into a nice class?
                            //TODO: consider offset in here...
                            assert(iVtx < it->c_indices->getCount());
                            size_t c_idx = (*it->c_indices)[iVtx];
                            size_t c_stride = it->c_stride;
                            size_t c_length = it->c_length;
                            const float* c_data = it->c_data->getFloatValues()->getData();

                            assert(it->rtr_layer->num_components() == c_length);

                            //boundary checks
                            size_t upper_bound = c_idx * c_stride + c_length -1;
                            if (upper_bound >= it->c_data->getFloatValues()->getCount() ) {
                                cout << "Error: Layer '" << it->rtr_layer->name() << 
                                        "' of mesh '" << rtr_mesh_info.rtr_mesh->id() <<
                                        "' contains an invalid index '" <<
                                        c_idx << "'. Setting this element to zero." << endl;
                                //Copy actual data
                                for ( size_t iComponent = 0; 
                                      iComponent<c_length;
                                      ++iComponent )
                                {
                                    it->rtr_layer_src->add_float_data(0);
                                    assert( it->rtr_layer_src->float_data_size() ==
                                            (_idx_count*c_length + iComponent+1) );
                                }
                            } else {

                                //Copy actual data
                                for ( size_t iComponent = 0; 
                                      iComponent<c_length;
                                      ++iComponent )
                                {

                                    size_t idx = c_idx * c_stride + iComponent;

                                    float new_datum = c_data[idx];

                                    it->rtr_layer_src->add_float_data(new_datum);
                                    assert( it->rtr_layer_src->float_data_size() ==
                                            (_idx_count*c_length + iComponent+1) );
                                }

                            }

                        } // assembly-loop

                        //use the new index, and increment
                        rtr_mesh_info.rtr_mesh->add_index_data(_idx_count);

                        //now memorize this setting
                        IdxLookup::value_type entry(c_idx_set, _idx_count);
                        _idx_cache.insert(entry);
                        _idx_count++;

                    } // new index-creation block

                } // vertex-index loop

                //save the vertex count
                int vtx_count = vtx_ass.rtr_layer_src->float_data_size() / 3;
                rtr_mesh_info.rtr_mesh->set_vertex_count(vtx_count);

                calculate_bounding_volumes(*rtr_mesh_info.rtr_mesh);

                //The material id will be resolved in the post-process stage
                //of the visual scene processor.
                MeshToMaterialMap::value_type mmp(rtr_mesh_info.rtr_mesh->id(), 
                                                  c_prim->getMaterialId());
                _bake_cache.rtr_meshes.insert(mmp);

                //add the created to the list of processed meshes
                _mesh_infos.push_back(rtr_mesh_info);

            } else {
                cout << "Primitive Type not (yet) supported." << endl;
                cout << "Import of geometry " << _c_mesh_id << " might be ";
                cout << " incomplete" << endl;
            }

        } // primitive-loop

        //memorize this conversion with the bake cache
        BakerCache::GeometryBakeCache::value_type v(c_geo->getUniqueId(),
                                                    _bake_cache);
        bool success = _baker->cache().geometries.insert(v).second;

        assert(success);

        //register post process stage
        //Note that the GeometryProcessor's post process stage has tun run
        //after the EffectProcessor and MaterialProcessor post process stage.
        _baker->register_for_postprocess(shared_from_this(), 5);

    } else {
        cout << "Type: " << c_geo->getType() << " not supported." << endl;
    }

    return true;
}

void GeometryProcessor::calculate_tangent_layer(rtr_format::Mesh& rtr_mesh) {

    //This code is based on:
    //Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary 
    //Mesh”. Terathon Software 3D Graphics Library, 2001. 
    //http://www.terathon.com/code/tangent.html

    if (rtr_mesh.primitive_type() != rtr_format::Mesh::TRIANGLES) {
        cout << "Error: Tangent layer generation is only supported for " 
             << " TRIANGLES primitive at the moment. Not generating tangents."
             << endl << endl;
        return;
    }

    if (rtr_mesh.index_data().size() == 0) {
        cout << "Error: Tangent layer generation is only supported for "
             << "indexed meshes. Not generating tangents." << endl;
        return;
    }

    if (_layer_sources.count(kNormalsLayerName()) == 0) {
        cout << "Error: Cannot calculate tangent layers for " 
                << "mesh '" << rtr_mesh.id() << "' because we don't"
                << " have normals available." << endl << endl;
        return;
    }

    //get the UV layer that is used for normal mapping. The source data of 
    //this layer is used for texture-tangent space calculation, and we also
    //need eventual stride information (eg. if we have 3D coords, as exported).
    const rtr_format::Mesh_VertexAttributeLayer * tex_mesh_lyr = NULL;

    for (int i = 0; i<rtr_mesh.layer_size(); ++i) {
        if (rtr_mesh.layer().Get(i).name() == kUvNormalMapName()) {
            tex_mesh_lyr = &rtr_mesh.layer().Get(i);
        }
    }

    //We always calculate tangents for the first (default) UV coord layer
    if (tex_mesh_lyr == NULL) {
        cout << "Error: Cannot calculate tangent layers for " 
                << "mesh '" << rtr_mesh.id() << "' because we don't"
                << " a normal map UV coord available." << endl << endl;
        return;
    }

    const rtr_format::LayerSource * tex_layer = NULL;

    //find the actual layer source (NOTE: we can't use a map lookup in here, 
    //as the layer name is not tracked in the actual cache key).
    LayerSourceCache::const_iterator it_lyr_src;
    for (it_lyr_src = _layer_sources.begin();
         it_lyr_src != _layer_sources.end();
         ++it_lyr_src)
    {
        if (it_lyr_src->second.id() == tex_mesh_lyr->source()) {
            tex_layer = &it_lyr_src->second;
        }
    }

    assert(tex_mesh_lyr != NULL);
    assert(tex_layer != NULL);

    int tex_stride = tex_mesh_lyr->num_components();

    int vertex_count = rtr_mesh.vertex_count();
    int triangle_count = rtr_mesh.index_data_size() / 3;

    const rtr_format::LayerSource * vtx_layer = 
                                        &_layer_sources[kPositionsLayerName()];
    
    const rtr_format::LayerSource * nml_layer = 
                                          &_layer_sources[kNormalsLayerName()];

    //Initialize the tangent layer, this is 4-component layer (as used by
    //our sources), where the fourth component represents the handedness
    //which saves us from storing the bitangent layer explicitly
    rtr_format::LayerSource new_layer;
    new_layer.set_id(rtr_mesh.id() + "_tangents");
    new_layer.set_type(rtr_format::LayerSource::FLOAT);

    boost::scoped_array<vec3> tangent_data(new vec3[vertex_count * 2]);
    vec3* tan1 = &tangent_data[0];
    vec3* tan2 = tan1 + vertex_count;

    std::fill(&tan1[0], &tan1[vertex_count*2], vec3(0) );
    
    bool tex_coords_degenerate_warning = false;

    for (long a = 0; a < triangle_count; a++)
    {
        unsigned int i1 = rtr_mesh.index_data().Get(a*3);
        unsigned int i2 = rtr_mesh.index_data().Get(a*3+1);
        unsigned int i3 = rtr_mesh.index_data().Get(a*3+2);
        
        vec3 v1 = Utils::vec3_from_arr(vtx_layer->float_data(), i1);
        vec3 v2 = Utils::vec3_from_arr(vtx_layer->float_data(), i2);
        vec3 v3 = Utils::vec3_from_arr(vtx_layer->float_data(), i3);
        
        vec2 w1 = Utils::vec2_from_arr(tex_layer->float_data(), i1, tex_stride);
        vec2 w2 = Utils::vec2_from_arr(tex_layer->float_data(), i2, tex_stride);
        vec2 w3 = Utils::vec2_from_arr(tex_layer->float_data(), i3, tex_stride);
        
        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;
        
        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;
        
        float r = 1.0F / (s1 * t2 - s2 * t1);

        if (!boost::math::isfinite(r)) {
            tex_coords_degenerate_warning = true;
            r = 0;   
        }

        vec3 sdir( (t2 * x1 - t1 * x2) * r, 
                   (t2 * y1 - t1 * y2) * r,
                   (t2 * z1 - t1 * z2) * r );

        vec3 tdir( (s1 * x2 - s2 * x1) * r, 
                   (s1 * y2 - s2 * y1) * r,
                   (s1 * z2 - s2 * z1) * r );
        
        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;
        
        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
        
    }

    if (tex_coords_degenerate_warning) {
        cout << "Warning: '" << rtr_mesh.id() << "' contains degenerated "
             << "tex_coord values. Tangent space generation might have  "
             << "failed for all or some faces." << endl;
    }
    
    bool issue_final_tangent_warning = false;

    for (long a = 0; a < vertex_count; a++)
    {
        vec3 n = Utils::vec3_from_arr(nml_layer->float_data(), a);
        vec3 t = normalize(tan1[a]);
        
        //We do not need to apply Gram-Schmidt orthogonalisation
        //in order to use the tranpose as the inverse, since we do not
        //need to insverse our basis

        // Gram-Schmidt orthogonalize
        // tangent[a]
        // vec3 tangent = normalize(t - n * dot(n, t));

        //add fallback to avoid float specialties
        if (!boost::math::isfinite(t.x) ||
            !boost::math::isfinite(t.y) ||
            !boost::math::isfinite(t.z) ) {
            issue_final_tangent_warning = true;
            t = vec3(0, 0, 1);
        }

        if (t == vec3(0) ) {
            issue_final_tangent_warning = true;
            t = vec3(0, 0, 1);
        }

        new_layer.add_float_data(t.x);
        new_layer.add_float_data(t.y);
        new_layer.add_float_data(t.z);
        
        // Calculate handedness as W component
        float handedness = (dot(cross(n, t), tan2[a]) < 0.0f) ? -1.0f : 1.0f;
        new_layer.add_float_data(handedness);
    }

    if (issue_final_tangent_warning) {
        cout << "Warning: Tangent calculation of '" << rtr_mesh.id()
             << "' failed on some or all faces." << endl;
    }

    //calculation was successful, add to mesh
    LayerSourceCache::value_type v(kTangentsLayerName(), new_layer);
    rtr_format::LayerSource * tng_layer = &_layer_sources.insert(v).first->second;

    //add the layer to the mesh
    rtr_format::Mesh_VertexAttributeLayer * tng_mesh_lyr = rtr_mesh.add_layer();
    tng_mesh_lyr->set_name(kTangentsLayerName());
    tng_mesh_lyr->set_num_components(4);
    tng_mesh_lyr->set_source(tng_layer->id());
    tng_mesh_lyr->set_source_index(0);

}

void GeometryProcessor::calculate_bounding_volumes(rtr_format::Mesh& rtr_mesh) {
    
    //This algorithm requires indexes vertices
    if (_layer_sources.count(kPositionsLayerName()) == 0) {
        cout << "Error: Cannot calculate bounding volume for " 
                << "mesh '" << rtr_mesh.id() << "' because we don't"
                << " have positions available." << endl << endl;
        return;
    }

    const rtr_format::LayerSource * vtx_layer = 
                                        &_layer_sources[kPositionsLayerName()];

    vec3 centroid(0);
    vec3 aabb_min(std::numeric_limits<float>::infinity());
    vec3 aabb_max(-std::numeric_limits<float>::infinity());

    float radius = 0;

    //Note: we must use the indices to access the vertices, because we might
    //have multiple meshes indexing into the same vertex arra

    for (int i = 0; i < rtr_mesh.index_data_size(); ++i) {

        unsigned int idx = rtr_mesh.index_data().Get(i);
        //get the vertex
        vec3 vtx = Utils::vec3_from_arr(vtx_layer->float_data(), idx);

        centroid += vtx;

        aabb_min.x = glm::min(vtx.x, aabb_min.x);
        aabb_min.y = glm::min(vtx.y, aabb_min.y);
        aabb_min.z = glm::min(vtx.z, aabb_min.z);

        aabb_max.x = glm::max(vtx.x, aabb_max.x);
        aabb_max.y = glm::max(vtx.y, aabb_max.y);
        aabb_max.z = glm::max(vtx.z, aabb_max.z);

    }

    float centroid_div = 1.0f / (float)rtr_mesh.index_data_size();
    centroid = centroid * centroid_div;

    //iterate again to find the furthest vertex relative to the centroid
    for (int i = 0; i < rtr_mesh.index_data_size(); ++i) {
       
        unsigned int idx = rtr_mesh.index_data().Get(i);
        //get the vertex
        vec3 vtx = Utils::vec3_from_arr(vtx_layer->float_data(), idx);

        float dist = glm::distance(vtx, centroid);
        if (dist > radius)
            radius = dist;
    }

    if (boost::math::isinf(radius) || 
        boost::math::isnan(radius) ||
        (radius != radius) ) {  
        cout << "Error: Calculation of bounding sphere radius of geometry '"
             << rtr_mesh.id() << "' yields NaN or Inf. File might not load."
             << endl;
    }

    rtr_mesh.mutable_bounding_sphere()->set_center_x(centroid.x);
    rtr_mesh.mutable_bounding_sphere()->set_center_y(centroid.y);
    rtr_mesh.mutable_bounding_sphere()->set_center_z(centroid.z);

    rtr_mesh.mutable_bounding_sphere()->set_radius(radius);
}

void GeometryProcessor::check_fallback_layers(rtr_format::Mesh& rtr_mesh) {

    //We check if a required layer is missing, if its is we add one, with
    //dummy data and issue a warning.
    //Note: this might be replaced with something more meaningful like
    //actual normals / UV / tangent generation code. For now we just ensure
    //that shaders won't screw up too badly.

    //Also note that for constant colors (where we use 1px textures) and
    //default lightmaps (black), the UV coords really doesn't matter

    std::set<string> mesh_layers;
    for (int i = 0; i<rtr_mesh.layer_size(); ++i) {
        const rtr_format::Mesh_VertexAttributeLayer& mesh_lyr = 
            rtr_mesh.layer().Get(i);

        if (!mesh_layers.insert(mesh_lyr.name()).second) {
            cout << "Error: Mesh '" << rtr_mesh.id() << "' contains duplicate"
                 << " layer definition of '" << mesh_lyr.name() << "'."
                 << endl;
            _baker->fail();
        }
    }

    if (mesh_layers.count(kUvLayerName()) == 0) {
        add_padded_layer(rtr_mesh, kUvLayerName(), 2, 0);
    }

    if (mesh_layers.count(kNormalsLayerName()) == 0) {
        add_padded_layer(rtr_mesh, kNormalsLayerName(), 3, 0);
    }

    if (mesh_layers.count(kUvDiffuseName()) == 0) {
        add_padded_layer(rtr_mesh, kUvDiffuseName(), 2, 0);
    }

    if (mesh_layers.count(kUvSpecularName()) == 0) {
        add_padded_layer(rtr_mesh, kUvSpecularName(), 2, 0);
    }

    if (mesh_layers.count(kUvNormalMapName()) == 0) {
        add_padded_layer(rtr_mesh, kUvNormalMapName(), 2, 0);
    }

    //if (mesh_layers.count(kUvLightMapName()) == 0) {
    //    add_padded_layer(rtr_mesh, kUvLightMapName(), 2, 0);
    //}

    if (mesh_layers.count(kUvAmbientMapName()) == 0) {
        add_padded_layer(rtr_mesh, kUvAmbientMapName(), 2, 0);
    }

    if (mesh_layers.count(kTangentsLayerName()) == 0) {
        add_padded_layer(rtr_mesh, kTangentsLayerName(), 4, 1);
    }

}

void GeometryProcessor::add_padded_layer(rtr_format::Mesh& rtr_mesh, 
                                         const string& name,
                                         int num_components,
                                         float pad_data)
{

    LayerSourceCache::value_type val(name, rtr_format::LayerSource());

    rtr_format::LayerSource * new_layer = 
        &_layer_sources.insert(val).first->second;

    new_layer->set_id(rtr_mesh.id() + "_" + name + "_fallback");
    new_layer->set_type(rtr_format::LayerSource::FLOAT);

    //add the layer to the mesh
    rtr_format::Mesh_VertexAttributeLayer * mesh_layer = rtr_mesh.add_layer();
    mesh_layer->set_name(name);
    mesh_layer->set_num_components(num_components);
    mesh_layer->set_source(new_layer->id());
    mesh_layer->set_source_index(0);

    //Add the data
    for (int i = 0; i<rtr_mesh.vertex_count() * num_components; ++i)
        new_layer->add_float_data(pad_data);

    cout << "Warning: Geometry '" << rtr_mesh.id() << "' had no layer '"
            << name << "' defined. Creating a dummy layer with values of (";

    for (int i = 0; i<num_components; ++i) {
        if (i != 0)
            cout << ", ";

        cout << pad_data;
    }

    cout << "). Rendering results might be unexpected." << endl << endl;
}

void GeometryProcessor::assembly_to_multi_index( const LayerAssemblyArray& ass, 
                                                 MultiIndex& out, 
                                                 int idx ) 
{
    if (out.size() != ass.size())
        out.resize(ass.size());

    for (size_t i = 0; i<ass.size(); ++i) {
        out[i] = ass[i].c_indices != NULL ? (*ass[i].c_indices)[idx] : 0;
    }

}

GeometryProcessor::LayerAssembly 
GeometryProcessor::setup_layer( const string& layer_name, 
                                rtr_format::Mesh * mesh,
                                const CF::MeshVertexData& c_data,
                                const CF::UIntValuesArray* c_indices,
                                size_t c_sub_index)
{

    rtr_format::LayerSource * layer_source = NULL;

    //If we haven't initialize a layer with the requested name yet, 
    //create one
    if (_layer_sources.find(layer_name) == _layer_sources.end()) {

        //create this layer_source
        rtr_format::LayerSource rtr_layer;
        rtr_layer.set_id(_c_mesh_id + "_" + layer_name);
        //Note: we do not support int layers at the moment. If we do, we should
        //take care of this in here as well
        rtr_layer.set_type(rtr_format::LayerSource::FLOAT);
        LayerSourceCache::value_type entry( layer_name, 
                                            rtr_layer );

        layer_source = & _layer_sources.insert(entry).first->second;

    } else {
        layer_source = &_layer_sources[layer_name];
    }

    //Obviously, length is NOT the length of a parameter tuple, but 
    //describes the length of the array being accessed. The case where
    //stride does not equal the number of parameters in an accessor is not
    //clearly stated in OpenCOLLADA user documentation, therefore, we will
    //assume that these cases (which are in fact rare) won't occur.

    //size_t c_length = c_data.getLength(c_sub_index);
    size_t c_stride = c_data.getStride(c_sub_index);
    size_t c_length = c_stride;

    size_t length = 0;
    size_t stride = 0;

    //workaround for OpenCOLLADA bug, where length and stride is not correctly
    //reported.
    if ( (layer_name == kPositionsLayerName()) || 
         (layer_name == kNormalsLayerName()) )
    {
        length = 3;
        stride = ( c_stride != 0 ? c_stride : 3 );
    } else {
        length = c_length;
        stride = c_stride;
    }

    assert( (length != 0) && (stride != 0) );

    rtr_format::Mesh_VertexAttributeLayer * lyr = mesh->add_layer();
    lyr->set_name(layer_name);
    lyr->set_num_components(length);
   
    lyr->set_source(_c_mesh_id + "_" + layer_name);

    //Note: with the current way of importing geoemtry, the case where we have
    //multiple layers that need an offset is not possible, therefore the offset
    //is always zero!
    lyr->set_source_index(0);

    //finally put together the assembly object and return it
    LayerAssembly assembly( c_indices,
                            &c_data,
                            length,
                            stride,
                            c_sub_index,
                            layer_source,
                            lyr );

    return assembly;
}

bool GeometryProcessor::post_process() {

    MeshInfoList::iterator it;
    for (it = _mesh_infos.begin();
         it != _mesh_infos.end();
         ++it)
    {

        //get the original collada material id
        MeshToMaterialMap::const_iterator it_mat = 
            _bake_cache.rtr_meshes.find(it->rtr_mesh->id());

        if (it_mat == _bake_cache.rtr_meshes.end()) {
            cout << "Error: Could not look up own BakeCache in Geometry." 
                 << endl;
            return false;
        }
        
        CF::MaterialId c_mat_id = it_mat->second;

        //Lookup the texture map binding from visual scene cache
        BakerCache::SceneMaterialBindingMap::key_type key(_c_id, c_mat_id);

        BakerCache::SceneMaterialBindingMap::const_iterator it_mapping;
        it_mapping = _baker->cache().scene_material_cache.find(key);

        if (it_mapping == _baker->cache().scene_material_cache.end()) {
            cout << "Error: Could not look up texture binding in BakerCache "
                 << endl;
            return false;
        }

        const BakerCache::VisualSceneMaterialBinding& binding_cache = 
            it_mapping->second;

        const BakerCache::TexCoordBindingList& c_tex_coord_bindings = 
            binding_cache.tex_coord_binding_list;

        //get the material-cache
        BakerCache::MaterialBakeCache::const_iterator it_mat_cache = 
            _baker->cache().materials.find(binding_cache.referenced_material);

        if (it_mat_cache == _baker->cache().materials.end()) {
            cout << "Could not look up material cache from GeometryProcessor."
                 << endl;
            return false;
        }

        const EffectProcessor::LayerToTextureMapInfoMap& mat_layer_to_map_info = 
            it_mat_cache->second.layer_to_map_info;

        EffectProcessor::LayerToTextureMapInfoMap::const_iterator it_lyr_to_id;

        for (it_lyr_to_id = mat_layer_to_map_info.begin();
             it_lyr_to_id != mat_layer_to_map_info.end();
             ++it_lyr_to_id)
        {
            const string& lyr_bound_name = it_lyr_to_id->first;
            const EffectProcessor::TextureMapInfo& map_info = it_lyr_to_id->second;

            const CF::TextureMapId& c_map_id = map_info.c_texture_map_id;
            const string& c_map_id_string = map_info.c_texture_map_string;
            
            //We have to decide between two lookup-modes: 
            //1) We are looking up the mapping for a COLLADA standard texture, 
            //   in this case, OpenCOLLADA uses TextureMapId to express the
            //   bindings.
            //2) We are looking up a texcoord binding for a texture that was
            //   only accessible via extra tags (e.g. bump texture from
            //   3DSMax OpenCOLLADA profile. In this case, we will use the
            //   semantic string to pick out the correct binding.
            //For the latter case, the value of c_map_id will be 
            //kInvalidTextureMapId

            //In our binding array, find the binding containing our
            //texture map ID, and lookup which UV set_idx is bound
            //to it for this material.
            const size_t invalid_set_idx = std::numeric_limits<size_t>::max();
            size_t set_idx = invalid_set_idx;
            BakerCache::TexCoordBindingList::const_iterator it_bdg;
            for (it_bdg = c_tex_coord_bindings.begin(); 
                 it_bdg != c_tex_coord_bindings.end();
                 ++it_bdg)
            {
                if (c_map_id != EffectProcessor::kInvalidTextureMapId()) {
                    //Mode 1 for standard textures as managed by OpenCOLLADA
                    if (it_bdg->getTextureMapId() == c_map_id) {
                        set_idx = it_bdg->getSetIndex();
                    }
                } else {
                    //Mode 2 for textures we extracted from extra tags
                    if (it_bdg->getSemantic() == c_map_id_string)
                        set_idx = it_bdg->getSetIndex();
                }
            }

            if ( (set_idx == invalid_set_idx) && 
                 !c_map_id_string.empty()) {
                //There is a bug in the OpenCOLLADA exporter, where the correct
                //semantic binding might not be exported... therefore we try
                //the following: if we have a semantic like CHANNELX, we use
                //as regex to extract the number.

                const boost::regex regex("^CHANNEL(\\d)$");

                boost::match_results<std::string::const_iterator> match;
                if (boost::regex_match(c_map_id_string, match, regex)) {
                    string str_match = match[1];
                    std::stringstream ss(str_match);
                    int set_idx_ss = 0;
                    if (! (ss >> set_idx_ss).fail()) {
                        set_idx_ss--;
                        cout << "Warning: Applying workaround to interpret '" 
                             << c_map_id_string << "' as referencing UV set idx"
                             << " " << set_idx_ss << endl;
                        set_idx = set_idx_ss;
                    }
                }
            }

            if (set_idx == invalid_set_idx) {

                //Fallback cases
                size_t fallback_idx = 0;

                //This is basically the old case, where we are not able
                //to catch esplicit mapping, and therefore need to fallback
                if (lyr_bound_name == kUvAmbientMapName())
                    fallback_idx = 1;

                //In this case we will fall back to dummy layers at a later 
                //point
                cout << "Warning: Could not find explicit texturemap to "
                        << "UV set idx binding for mesh '" 
                        << it->rtr_mesh->id() << "' with layer '"
                        << lyr_bound_name << "'. Falling back to set_idx = "
                        << fallback_idx << "."
                        << endl;
                set_idx = fallback_idx;
            }

            // get the local layer name for this set_idx
            SetIndexToNameMap::const_iterator it_find_lyr_name = 
                it->uv_set_to_layername.find(set_idx);
                
            if (it_find_lyr_name == it->uv_set_to_layername.end()) {
                //This happens, when a material tries to use a UV set
                //that the mesh does not have
                cout << "Error: Mesh '"
                        << it->rtr_mesh->id() << "' has no UV set '"
                        << set_idx << "'." << endl;
                continue;
            }

            // Get the lyr prototype with the just looked-up name
            NameToRTRVALayerMap::const_iterator it_find_lyr = 
                it->rtr_layer_cache.find(it_find_lyr_name->second);

            if (it_find_lyr == it->rtr_layer_cache.end()) {
                cout << "Error: Could not find local layer '"
                        << it_find_lyr_name->second << "' in mesh '"
                        << it->rtr_mesh->id() << "'." << endl;
                return false;
            }

            //finally, add a new layer, copy it from the prototype
            //(which then has its source setup properly, and set the layer
            //name as present in the binding

            rtr_format::Mesh_VertexAttributeLayer* rtr_layer = 
                it->rtr_mesh->add_layer();

            rtr_layer->CopyFrom(*it_find_lyr->second);

            rtr_layer->set_name(lyr_bound_name);
        }

        //If we have not had any explicit bump-texture coordinate mapping, we
        //we will fallback to the first texture channel


        // We calculate and add the tangent array to this mesh
        calculate_tangent_layer(*it->rtr_mesh);

        //If we are still missing some of our required layers, 
        //use those.
        check_fallback_layers(*it->rtr_mesh);

        //Write out this mesh
        bool b = _baker->write_baked(it->rtr_mesh->id(), it->rtr_mesh.get());
        if (!b) {
            cerr << "Baking mesh failed." << endl;
            return false;
        }

        //write out sources
        for ( LayerSourceCache::iterator it = _layer_sources.begin();
              it != _layer_sources.end();
              ++it )
        {
            bool b = _baker->write_baked(it->second.id(),&(it->second));
            if (!b)
                return false;
        }


    }

    return true;
}