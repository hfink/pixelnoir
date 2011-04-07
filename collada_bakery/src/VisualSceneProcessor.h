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

#ifndef __CB_VISUAL_SCENE_PROCESSOR_H
#define __CB_VISUAL_SCENE_PROCESSOR_H

#include "cbcommon.h"
#include "Types.h"

#include "Processor.h"

#include "rtr_format.pb.h"

#include "COLLADAFWNode.h"
#include "COLLADAFWUniqueId.h"

#include <set>

namespace ColladaBakery {

    class Baker;

    /**
     * This class collect all transforms, already adds them in the right order, 
     * and also, we require that our target scene objects already contains
     * dereferenced camera and light information.
     */
    class VisualSceneProcessor : public Processor {

    public:

        //We just need to memorize the IDs of the baked scenes.
        struct BakeCache {
            string id;
        };

        VisualSceneProcessor(Baker* baker);

        virtual bool process(const CF::Object* cObject);
        virtual bool post_process();

    private:

        rtr_format::Scene _rtr_scene;

        void process_node( const CF::Node* c_node, 
                           const string& dependency );

        //returns the name of the newly converted transforms
        //use the return value to set dependencies on other transform nodes
        void process_trafo(const CF::Transformation* c_trafo, 
                           rtr_format::TransformNode* t_node);

        string make_unique_trafo_name(const string& pref_id);

        string _scene_id;
        CF::UniqueId _c_scene_id;

        //for loading transformations within one particular scene
        NameSet _trafo_name_cache;
        
        //stuff we need for post processing

        struct ResolveData {
            string c_node_id;
            CF::UniqueId referenced_id;
            string rtr_node;
        };

        typedef map<CF::MaterialId, CF::MaterialBinding> MatBindingMap;
        struct GeometryResolveData {
            ResolveData resolve_data;
            //We structure this as a map between
            //MaterialId (of a mesh) to material binding
            MatBindingMap material_bindings;
        };
        
        struct NodeResolveData {
            ResolveData resolve_data;
            string rtr_dependency;
        };

        list<ResolveData> _camera_instances;
        list<GeometryResolveData> _geometry_instances;
        list<ResolveData> _light_instances;
        list<NodeResolveData> _node_instances;

        std::set<string> _nodes_name_cache;

    };

    typedef boost::shared_ptr<VisualSceneProcessor> VisualSceneProcessorRef;
}

#endif //__CB_VISUAL_SCENE_PROCESSOR_H