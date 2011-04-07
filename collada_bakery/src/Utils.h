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

#ifndef __CB_UTILITY_H
#define __CB_UTILITY_H

#include "Types.h"
#include "COLLADAFWUniqueId.h"
#include "COLLADAFWMesh.h"
#include "COLLADAFWScene.h"
#include "COLLADAFWNode.h"
#include "COLLADAFWVisualScene.h"
#include "COLLADAFWCamera.h"
#include "COLLADAFWAnimationCurve.h"
#include "COLLADAFWMaterial.h"
#include "COLLADAFWEffect.h"
#include "COLLADAFWEffectCommon.h"
#include "COLLADAFWTextureCoordinateBinding.h"

#include <sstream>

#include "rtr_format.pb.h"

namespace ColladaBakery { namespace Utils {

    inline string make_unique(const string& pref_name, NameSet& cache) {
        string name = pref_name;
        int i = 1;
        while (cache.find(name) != cache.end()) {
            std::ostringstream oss;
            oss << pref_name << "_" << i;
            name = oss.str();
            ++i;
        }

        //finally found one
        cache.insert(name);
    
        return name;
    }


    template <typename T>
    inline string c_type_to_str(const T * obj) {
        //This is the generic type, where we haven't defined
        //any specialization.
        std::ostringstream oss; 
        oss << "class_" << T::ID();
        return oss.str();
    }

    template<>
    inline string c_type_to_str(const CF::Mesh *) {
        return "mesh";
    }

    template<>
    inline string c_type_to_str(const CF::VisualScene *) {
        return "scene";
    }

    template<>
    inline string c_type_to_str(const CF::Node *) {
        return "node";
    }

    template<>
    inline string c_type_to_str(const CF::Camera *) {
        return "camera";
    }

    template<>
    inline string c_type_to_str(const CF::AnimationCurve *) {
        return "anim_curve";
    }

    template<>
    inline string c_type_to_str(const CF::Material *) {
        return "material";
    }

    template<>
    inline string c_type_to_str(const CF::Effect *) {
        return "effect";
    }

    template<>
    inline string c_type_to_str(const CF::EffectCommon *) {
        return "effect_common";
    }

    //This helper method finds a per-document unique identifier that is
    //easy to read and to use as a filename. We prefer the original ID which
    //is per definition of a COLLADA ID unique within its document. If we can 
    //not reach this ID, we use the more generic UniqueId, but we try to 
    //translate the typed parameter to a more redable specialization, if 
    //available.
    //Note: As it seems, COLLADA ID do not guarantee unique naming. There might
    //be multiple animations that carry the same ID withing the OpenCOLLADA
    //Framework that have their origin in a unique element, but shall be split
    //in the framework. Therefore we will additionally add a cache that
    //takes care of making the name unique globally.
    template <typename T>
    inline string get_id(const T * c_element, NameSet& cache) {
        std::ostringstream oss;
        string id;
        
        if (!c_element->getOriginalId().empty()) {
            id = c_element->getOriginalId();
        } else {
            string type_str = c_type_to_str(c_element);
            oss << type_str << "_" 
                << c_element->getUniqueId().getObjectId();
            id = oss.str();
        }

        //make it unique
        id = make_unique(id, cache);

        return id;
    }

    template <>
    inline string get_id(const CF::VisualScene * c_element, NameSet& cache) {
        std::ostringstream oss;
        string id;
        
        string type_str = c_type_to_str(c_element);
        oss << type_str << "_" 
            << c_element->getUniqueId().getObjectId();
        id = oss.str();

        //make it unique
        id = make_unique(id, cache);

        return id;
    }

    //Handy function to access vec3, vec4 from a google protobuf array
    inline vec3 vec3_from_arr(const google::protobuf::RepeatedField<float>& a,
                              int idx)
    {
        assert(idx*3 + 2 < a.size());
        return vec3(a.Get(idx*3), 
                    a.Get(idx*3+1), 
                    a.Get(idx*3+2) );
    }

    inline vec2 vec2_from_arr(const google::protobuf::RepeatedField<float>& a,
                              int idx, int stride = 2)
    {
        assert(idx*stride + 1 < a.size());
        return vec2(a.Get(idx*stride), 
                    a.Get(idx*stride+1) );
    }

} } //namespace Utils namespace ColladaBakery

 inline bool epsilon_compare(float a, float b) {
    static const float EPSILON = 0.001f;
    return ( ((b-EPSILON) < a) &&
                ((b+EPSILON) > a) );
}

inline bool operator==(const mat4& lhs, const mat4& rhs) {

    bool equal = true;
    for (unsigned int i = 0; i<4; i++) {
        for (unsigned int j = 0; j<4; j++) {
            equal &= (epsilon_compare(lhs[i][j], rhs[i][j]) );
        }
    }
    return equal;
}

inline bool operator!=(const mat4& lhs, const mat4& rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator==(const vec4& lhs, const vec4& rhs) {

    bool equal = true;
    for (unsigned int i = 0; i<4; i++) {
        equal &= (epsilon_compare(lhs[i], rhs[i]) );
    }
    return equal;
}

inline bool operator!=(const vec4& lhs, const vec4& rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator==(const vec3& lhs, const vec3& rhs) {

    bool equal = true;
    for (unsigned int i = 0; i<3; i++) {
        equal &= (epsilon_compare(lhs[i], rhs[i]) );
    }
    return equal;
}

inline bool operator!=(const vec3& lhs, const vec3& rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator==(const CF::TextureCoordinateBinding& lhs, 
                       const CF::TextureCoordinateBinding& rhs) 
{
    return ( ( lhs.getSemantic() == rhs.getSemantic() ) &&
             ( lhs.getSetIndex() == rhs.getSetIndex() ) && 
             ( lhs.getTextureMapId() == rhs.getTextureMapId() ) );
}

inline bool operator!=(const CF::TextureCoordinateBinding& lhs, 
                       const CF::TextureCoordinateBinding& rhs) 
{
    return !operator==(lhs, rhs);
}

inline bool c_tex_coord_bgd_predicate(const CF::TextureCoordinateBinding& lhs, 
                                      const CF::TextureCoordinateBinding& rhs)
{
    return lhs == rhs;
}




#endif //__CB_UTILITY_H