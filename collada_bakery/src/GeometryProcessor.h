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

#ifndef __CB_GEOMETRY_PROCESSOR_H
#define __CB_GEOMETRY_PROCESSOR_H

#include "cbcommon.h"
#include "Processor.h"
#include "MeshMultiIndex.h"
#include "rtr_format.pb.h"
#include "COLLADAFWMeshVertexData.h"
#include "COLLADAFWTypes.h"

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

namespace ColladaBakery {

    class Baker;

    //Note: we do not support cross-geometry input sources. They would be result
    //in a correct image, but we will have to split them
    //note also that this would not be supported by opencollada?
    class GeometryProcessor : public Processor  {

    public:

        GeometryProcessor(Baker* baker);

        typedef map<string, CF::MaterialId > MeshToMaterialMap;
        //This cache includes holds a list of mesh_id -> material_id pairs
        struct BakeCache {
             MeshToMaterialMap rtr_meshes;
        };

        static const string& kPositionsLayerName() {
            static const string s = "vertex";
            return s;
        }

        static const string& kNormalsLayerName() {
            static const string s = "normal";
            return s;
        }

        static const string& kTangentsLayerName() {
            static const string s = "tangent";
            return s;
        }

        //Note: multiple, sequenced color indices will be named
        //      color_N
        static const string& kColorsLayerName() {
            static const string s = "color";
            return s;
        }

        //Note: multiple, sequenced color indices will be named
        //      tex_coord_M
        static const string& kUvLayerName() {
            static const string s = "tex_coord";
            return s;
        }

        //By convention our runtime uses a UV channel for each texture
        //and these are named as follows
        static const string& kUvDiffuseName() {
            static const string s = "uv_diffuse";
            return s;
        }

        static const string& kUvSpecularName() {
            static const string s = "uv_specular";
            return s;
        }

        static const string& kUvNormalMapName() {
            static const string s = "uv_normalmap";
            return s;
        }

        static const string& kUvLightMapName() {
            static const string s = "uv_lightmap";
            return s;
        }

        static const string& kUvAmbientMapName() {
            static const string s = "uv_ambientmap";
            return s;
        }

        virtual bool process(const CF::Object* cObject);

        /**
         * We use the post process of the geometry to resolve the UV->TEX
         * mappings that have been determined by the EffectProcessor and 
         * VisualSceneProcessor
         */
        virtual bool post_process();

    private:

        //cache for converting multiple indices to our commonly indexed layers
        typedef vector<UInt> MultiIndex;
        typedef boost::unordered_map<MultiIndex, UInt> IdxLookup;
        //cache for local layer sources. local name -> rtr_format layer source
        typedef boost::unordered_map<string, rtr_format::LayerSource > LayerSourceCache;

        typedef list<rtr_format::Mesh> MeshList;

        //Note: OpenCOLLADA does not report stride and length for 
        //vertices and normals correctly, therefore we explicitly pass them
        //with the Assembly and work around these cases
        //this type is used during output assembly
        struct LayerAssembly {
            const CF::UIntValuesArray * c_indices;
            const CF::MeshVertexData * c_data;
            size_t c_length;
            size_t c_stride;
            //TODO: if we provide length and stride explicitly, we might not
            //need c_num_layer explicitly...
            size_t c_num_layer; // for multiple clrs or Uv layers
            rtr_format::LayerSource * rtr_layer_src;
            rtr_format::Mesh_VertexAttributeLayer * rtr_layer;

            LayerAssembly() : 
                c_indices(NULL), 
                c_data(NULL), 
                c_length(0),
                c_stride(0),
                c_num_layer(0), 
                rtr_layer_src(NULL),
                rtr_layer(NULL) {}

            LayerAssembly( const CF::UIntValuesArray* c_indices_, 
                           const CF::MeshVertexData* c_data_,
                           size_t c_length_,
                           size_t c_stride_,
                           size_t c_num_layer_,
                           rtr_format::LayerSource* rtr_layer_src_,
                           rtr_format::Mesh_VertexAttributeLayer* rtr_layer_) : 
                c_indices(c_indices_),
                c_data(c_data_),
                c_length(c_length_),
                c_stride(c_stride_),
                c_num_layer(c_num_layer_),
                rtr_layer_src(rtr_layer_src_),
                rtr_layer(rtr_layer_){}
                
        };

        //typedef boost::tuple< const CF::UIntValuesArray* , 
        //                      rtr_format::LayerSource* > LayerAssembly;

        typedef vector<LayerAssembly> LayerAssemblyArray;

        /**
         * Sets up layer data structures. Instead of directly copying data, 
         * this method only initializes the data, and returns a layer assembly,
         * which can be used later to re-index the individual attribute layers.
         * If a layer with a particular name already exists, this layer will be 
         * used, if not, a new layer will be created.
         * 
         * @param layer_name The name of the vertex layer to set up.
         * @param mesh The mesh where a new vertex layer should be added to.
         * @param c_data The COLLADA source data that is referenced by the new
         * layer created via this method.
         * @param c_indices The COLLADA indices that represent the COLLADA 
         * layer data.
         * @param c_sub_index COLLADA indices can have multiple representations
         * (like multiple UV coords). Use this index to specify a layer other
         * than the first (default) one.
         * @return A LayerAssembly that contains all information to reindex
         * and fill the target layers with actual data.
         */
        LayerAssembly setup_layer( const string& layer_name, 
                                   rtr_format::Mesh * mesh,
                                   const CF::MeshVertexData& c_data,
                                   const CF::UIntValuesArray* c_indices,
                                   size_t c_sub_index = 0);

        //helper method
        void assembly_to_multi_index( const LayerAssemblyArray& ass, 
                                      MultiIndex& out, 
                                      int idx);

        //calculates the tangent array and adds it properly
        void calculate_tangent_layer(rtr_format::Mesh& rtr_mesh);

        //populates the bounding volume infor about a mesh
        void calculate_bounding_volumes(rtr_format::Mesh& rtr_mesh);

        //Adds fallback layers, if a required layer does not exist.
        void check_fallback_layers(rtr_format::Mesh& rtr_mesh);
        void add_padded_layer(rtr_format::Mesh& rtr_mesh, 
                              const string& name,
                              int num_components,
                              float pad_data);

        IdxLookup _idx_cache;
        unsigned int _idx_count;
        string _c_mesh_id;
        LayerSourceCache _layer_sources;
        MeshList _meshes;

        //data structure for UV coord determination
        typedef map<size_t, string> SetIndexToNameMap;

        typedef map<string, rtr_format::Mesh_VertexAttributeLayer *>
            NameToRTRVALayerMap;

        typedef boost::shared_ptr<rtr_format::Mesh> RTRMeshRef;

        struct MeshInfo {
            RTRMeshRef rtr_mesh;
            SetIndexToNameMap uv_set_to_layername;
            NameToRTRVALayerMap rtr_layer_cache;
        };

        typedef list<MeshInfo> MeshInfoList;
        MeshInfoList _mesh_infos;

        BakeCache _bake_cache;

        CF::UniqueId _c_id;

    };

    typedef boost::shared_ptr<GeometryProcessor> GeometryProcessorRef;
}

#endif //__CB_GEOMETRY_PROCESSOR_H