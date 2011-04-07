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

#ifndef __CB_BAKER_CACHE_H
#define __CB_BAKER_CACHE_H

#include "cbcommon.h"

#include "Types.h" 
#include "COLLADAFWUniqueId.h"
#include "COLLADAFWAnimationList.h"
#include "GeometryProcessor.h"
#include "CameraProcessor.h"
#include "LightProcessor.h"
#include "VisualSceneProcessor.h"
#include "MaterialProcessor.h"
#include "EffectProcessor.h"
#include "ImageProcessor.h"

#include <boost/unordered_map.hpp>

#include <set>

namespace ColladaBakery {
    
    class BakerCache {
    
    public:

        typedef boost::unordered_map< CF::UniqueId, 
                                      GeometryProcessor::BakeCache > 
                                                            GeometryBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      CameraProcessor::BakeCache > 
                                                            CameraBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      LightProcessor::BakeCache > 
                                                            LightBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      VisualSceneProcessor::BakeCache > 
                                                           VisualSceneBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      MaterialProcessor::BakeCache >
                                                             MaterialBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      EffectProcessor::BakeCache >
                                                               EffectBakeCache;

        typedef boost::unordered_map< CF::UniqueId, 
                                      ImageProcessor::BakeCache >
                                                               ImageBakeCache;

        //This is the cache where we cache animation mappings that we actually
        //want to import.
        //AnimationListID -> rtr target string
        //Note that this is e.g. written by the VisualSceneProcessor in-place
        //process method
        //Also note that more than one animation list could animate a rtr target
        //Note that the string is just the rtr target node, addressing of sub
        //elements still has to be resolved.
        typedef std::multimap< CF::UniqueId, string > AnimListToRTRTargetMap;

        //AnimationID -> rtr_target string + animationbinding (to resolve 
        //subscript, etc...)
        
        struct RTRBinding {
            string rtr_id;
            CF::UniqueId c_anim_list_id;
            CF::AnimationList::AnimationBinding c_anim_binding;
        };

        typedef std::multimap<CF::UniqueId, RTRBinding > AnimToRTRBindingMap;

        typedef list<CF::UniqueId> UniqueIdList;

        //AnimListID-> resulting RTR animation IDs
        //We use this cache to finally instantiate the animations in our
        //scene
        typedef map<CF::UniqueId, list<string> > AnimListToRTRAnimMap;

        GeometryBakeCache geometries;
        CameraBakeCache cameras;
        LightBakeCache lights;
        VisualSceneBakeCache scenes;
        MaterialBakeCache materials;
        EffectBakeCache effects;
        ImageBakeCache images;
        
        AnimListToRTRTargetMap animation_binding_requests;
        AnimToRTRBindingMap animation_resolved_bindings;
        AnimListToRTRAnimMap animation_binding_results;
        
        //TODO: maybe this list could be removed in future
        UniqueIdList animation_used_animlists;

        //We will save the instantiated instance_visual_scene of the <scene>
        //element in here (if present)
        CF::UniqueId startup_scene;

        //This is a cache that ensure unique naming within one document
        //While most COLLADA IDs are ensured to be unique within their
        //document, we might run into unnamed nodes for which we have to
        //generate a unique name
        NameSet id_cache;

        //A set of material IDs that were actually instantiated from a 
        //visual scene. We can filter our import of materials based on this
        //list. We don't want to import unused materials
        typedef std::set<CF::UniqueId> UniqueIdSet;

        UniqueIdSet used_materials;

        UniqueIdSet used_images;

        //The following data structure are used by the GeometryProcessor post
        //processing stage. We will map certain UV channels to fit the 
        //wired connections. This is a bit convolute as UV coord mapping is 
        //part of the mesh definition in our model, instead of being a part
        //of the material definition.

        typedef boost::tuple<CF::UniqueId, CF::MaterialId> GeomMatIdPair;
        typedef list<CF::TextureCoordinateBinding > TexCoordBindingList;

        struct VisualSceneMaterialBinding {
            TexCoordBindingList tex_coord_binding_list;
            CF::UniqueId referenced_material;
        };

        typedef map<GeomMatIdPair, VisualSceneMaterialBinding > 
            SceneMaterialBindingMap;

        SceneMaterialBindingMap scene_material_cache;
    };

}

#endif //__CB_BAKER_CACHE_H
