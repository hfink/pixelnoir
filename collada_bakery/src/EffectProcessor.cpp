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

#include "EffectProcessor.h"

#include "Baker.h"

#include "COLLADAFWEffect.h"
#include "COLLADAFWEffectCommon.h"
#include "kcfile.h"
#include "common_const.h"
#include <boost/regex.hpp>

#include "ColladaBakeryConfig.h"

using namespace ColladaBakery;

EffectProcessor::EffectProcessor(Baker* baker) : 
Processor(baker)
{
}

bool EffectProcessor::process(const CF::Object* c_obj) {
    //We defer processing of effect as we want to double-check if textures
    //for particular effect channels are available. We might also be dependend
    //on <extra> tags which wouldn't be available in the in-place processing
    //stage

    const CF::Effect * c_effect = static_cast<const CF::Effect*>(c_obj);

    size_t c_cmn_fx_cnt = c_effect->getCommonEffects().getCount();

    _c_id = c_effect->getUniqueId();

    string c_fx_id = _baker->get_id(c_effect);

    //In fact we just want one common profile effect
    if (c_cmn_fx_cnt == 0) {
        cout << "Warning: Effect '" << c_fx_id << "' has no profile_COMMON " 
             << " representation. Skipping import." << endl;
        return true;
    }

    if (c_cmn_fx_cnt > 1) {
        cout << "Effect '" << c_fx_id << "' has multiple profile_COMMON " 
             << "representations. Importing only the first occurrence." 
             << endl;
    }

    const CF::EffectCommon * c_effect_cmn = c_effect->getCommonEffects()[0];

    //We deal with the effect instance later and make a copy.
    _c_fx_cmn = *c_effect_cmn;

    _rtr_material_prototype.set_id(c_fx_id);

    //register post process stage
    //We have to ensure that the effects post-processor stages run before
    //the material processor's post-process is executed.
    _baker->register_for_postprocess(shared_from_this(), 2);

    return true;
}

bool EffectProcessor::post_process() {

    //TODO: create a stricter mapping between COLLADA effects and our
    //internal material system
    //For now we just take what get and need
    
    //TODO: allow a switch between multiple materials, the bakery needs some 
    //knowledge on which parameters to draw for which type of material 
    //automatically. For now we just extract specular and diffuse texture and
    //some common parameters

    BakeCache c;

    _rtr_material_prototype.set_shader("Dusty");
    
    const CF::SamplerPointerArray& c_sampler = _c_fx_cmn.getSamplerPointerArray();

    bool fallback = false;

    //Diffuse texture

    rtr_format::Parameter* rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("diffuse_tex");
    rtr_param->set_type(rtr_format::TEXTURE);
    rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX() + "/stock_default.png");

    if (_c_fx_cmn.getDiffuse().isTexture()) {
        CF::SamplerID idx = _c_fx_cmn.getDiffuse().getTexture().getSamplerId();
        
        TextureMapInfo c_map_info;
        //memorize the texture map of this channel
        CF::TextureMapId c_map_id = 
            _c_fx_cmn.getDiffuse().getTexture().getTextureMapId();

        c_map_info.c_texture_map_id = c_map_id;

        LayerToTextureMapInfoMap::value_type v(GeometryProcessor::kUvDiffuseName(),
                                               c_map_info);

        if (!c.layer_to_map_info.insert(v).second) {
            cout << "Error could not insert to map-id to layer name cache of "
                 << "effect." << endl;
            return false;
        }

        CF::UniqueId src_id = c_sampler[idx]->getSourceImage();

        //Lookup if we have seen this image, and what its promised path is
        //going to be
        BakerCache::ImageBakeCache::const_iterator it_img = 
            _baker->cache().images.find(src_id);
        if (it_img != _baker->cache().images.end()) {

            rtr_param->set_svalue(it_img->second.img_path);
            c.used_images.push_back(src_id);

        } else {
            cout << "Referenced diffuse texture: " << src_id.toAscii() 
                 << " does not " 
                 << "exist." << endl;

            cout << "Using 'stock_default.jpg' instead." << endl;
        }
    } else if (_c_fx_cmn.getDiffuse().isColor()) {
        string color_tex = 
            get_string_for_color(_c_fx_cmn.getDiffuse().getColor());

        rtr_param->set_svalue(color_tex);
    }
   
    //Specular texture
    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("specular_tex");
    rtr_param->set_type(rtr_format::TEXTURE);
    rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX()+"/stock_black.png");

    if (_c_fx_cmn.getSpecular().isTexture()) {
        CF::SamplerID idx = _c_fx_cmn.getSpecular().getTexture().getSamplerId();
        CF::UniqueId src_id = c_sampler[idx]->getSourceImage();

        TextureMapInfo c_map_info;
        //memorize the texture map of this channel
        CF::TextureMapId c_map_id = 
            _c_fx_cmn.getSpecular().getTexture().getTextureMapId();

        c_map_info.c_texture_map_id = c_map_id;

        LayerToTextureMapInfoMap::value_type v(GeometryProcessor::kUvSpecularName(),
                                               c_map_info);

        if (!c.layer_to_map_info.insert(v).second) {
            cout << "Error could not insert to map-id to layer name cache of "
                 << "effect." << endl;
            return false;
        }

        //Lookup if we have seen this image, and what its promised path is
        //going to be
        BakerCache::ImageBakeCache::const_iterator it_img = 
            _baker->cache().images.find(src_id);
        if (it_img != _baker->cache().images.end()) {

            rtr_param->set_svalue(it_img->second.img_path);
            c.used_images.push_back(src_id);

        } else {
            
            cout << "Referenced specular texture: " << src_id.toAscii() 
                 << " does not " 
                 << "exist." << endl;

            cout << "Using 'stock_black.png' instead." << endl;
        }
    } else if (_c_fx_cmn.getSpecular().isColor()) {
        string color_tex = 
            get_string_for_color(_c_fx_cmn.getSpecular().getColor());

        rtr_param->set_svalue(color_tex);
    }

    //Normal Map

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("normal_tex");
    rtr_param->set_type(rtr_format::TEXTURE);
    rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX()+
                          "/stock_flat_nm.png");

    //Check if we have some useful data in OpenCOLLADA3dsmax extra tags
    string bump_tex = _baker->extra_handler().get_attribute(_c_id, 
                                          ExtraDataHandler::k3DSMaxProfile(),
                                          "bump/texture",
                                          "texture");

    //Note that we have modified the OpenCOLLADA lib in order to resolve
    //to samplers with a raw SID.
    TextureMapInfo c_map_info;
    if (!bump_tex.empty()) {
        const CF::Sampler* c_smpl = find_sampler(bump_tex);
        if (c_smpl != NULL) {
            CF::UniqueId src_id = c_smpl->getSourceImage();

            //Lookup if we have seen this image, and what its promised path is
            //going to be
            BakerCache::ImageBakeCache::iterator it_img = 
                _baker->cache().images.find(src_id);
            if (it_img != _baker->cache().images.end()) {
                //set the prospective path
                //AND add a suffix for normal maps, if still necessary
                //(this prevents multiple insertion)
                const boost::regex normalmap_pattern(rtr::kNormalMapFormat());

                string* img_dest = &it_img->second.img_path;
                if (!boost::regex_match(*img_dest, 
                                           normalmap_pattern))
                {
                    img_dest->insert(img_dest->size() - 4, "_nm");
                }

                rtr_param->set_svalue(*img_dest);
                c.used_images.push_back(src_id);

            } else {
                cout << "Referenced bump texture: " << src_id.toAscii() 
                     << " does not exist." << endl;
            }

            //Also, lookup up the texcoord semantic
            string c_tex_map = _baker->extra_handler().get_attribute(_c_id, 
                                                  ExtraDataHandler::k3DSMaxProfile(),
                                                  "bump/texture",
                                                  "texcoord");

            if (c_tex_map.empty()) {
                cout << "Error: Found bump texture, but could not extract "
                     << "the texcoord identifier." << endl;
                return false;
            }

            c_map_info.c_texture_map_string = c_tex_map;

        } else {
            cout << "Warning: COLLADA file might be corrupt, could not resolve"
                 << " to sampler '" << bump_tex << "'." << endl;
        }
    }

    LayerToTextureMapInfoMap::value_type val(GeometryProcessor::kUvNormalMapName(),
                                             c_map_info);

    if (!c.layer_to_map_info.insert(val).second) {
        cout << "Error could not insert to map-id to layer name cache of "
                << "effect." << endl;
        return false;
    }

    //Get the shininess
    float c_shininess = _c_fx_cmn.getShininess().getFloatValue();
    if (c_shininess == 0)
        bakery_config.dust_shininess();

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("shininess");
    rtr_param->set_type(rtr_format::FLOAT);
    rtr_param->add_fvalue(c_shininess);

    //Add the other default dusty-material parameters
    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("dust_thickness");
    rtr_param->set_type(rtr_format::FLOAT);
    rtr_param->add_fvalue(bakery_config.dust_thickness());

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("dust_min_angle");
    rtr_param->set_type(rtr_format::FLOAT);
    rtr_param->add_fvalue(bakery_config.dust_min_angle());

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("dust_exponent");
    rtr_param->set_type(rtr_format::FLOAT);
    rtr_param->add_fvalue(bakery_config.dust_exponent());

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("dust_color");
    rtr_param->set_type(rtr_format::VEC3);
    rtr_param->add_fvalue(bakery_config.dust_color().r);
    rtr_param->add_fvalue(bakery_config.dust_color().g);
    rtr_param->add_fvalue(bakery_config.dust_color().b);

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("ambient_occ_tex");
    rtr_param->set_type(rtr_format::TEXTURE);
    rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX()+
                          "/stock_white.png");

    rtr_param = _rtr_material_prototype.add_parameter();
    rtr_param->set_name("dust_noise_tex");
    rtr_param->set_type(rtr_format::TEXTURE);
    rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX()+
                          "/noise.png");

    //Handle the lightmap. For now we expect a lightmap to be a texture
    //in the emission channel (as exported from max)
    if (_c_fx_cmn.getAmbient().isTexture()) {
        CF::SamplerID idx = _c_fx_cmn.getAmbient().getTexture().getSamplerId();
        CF::UniqueId src_id = c_sampler[idx]->getSourceImage();

        TextureMapInfo c_map_info;
        //memorize the texture map of this channel
        CF::TextureMapId c_map_id = 
            _c_fx_cmn.getAmbient().getTexture().getTextureMapId();

        c_map_info.c_texture_map_id = c_map_id;

        LayerToTextureMapInfoMap::value_type v(GeometryProcessor::kUvAmbientMapName(),
                                               c_map_info);

        if (!c.layer_to_map_info.insert(v).second) {
            cout << "Error could not insert to map-id to layer name cache of "
                 << "effect." << endl;
            return false;
        }

        //Lookup if we have seen this image, and what its promised path is
        //going to be
        BakerCache::ImageBakeCache::const_iterator it_img = 
            _baker->cache().images.find(src_id);
        if (it_img != _baker->cache().images.end()) {

            rtr_param->set_svalue(it_img->second.img_path);
            c.used_images.push_back(src_id);

        } else {
            cout << "Referenced ambient texture: " << src_id.toAscii() 
                 << " does not " 
                 << "exist." << endl;

            cout << "Using 'stock_white.png' instead." << endl;
        }
        
    }

    //set the prospective path
    //rtr_param = _rtr_material_prototype.add_parameter();
    //rtr_param->set_name("lightmap_tex");
    //rtr_param->set_type(rtr_format::TEXTURE);
    //rtr_param->set_svalue(rtr::DEFAULT_TEXTURES_PATH_PREFIX()+
    //                      "/stock_black.png");

    ////Handle the lightmap. For now we expect a lightmap to be a texture
    ////in the emission channel (as exported from max)
    //if (_c_fx_cmn.getEmission().isTexture()) {
    //    CF::SamplerID idx = _c_fx_cmn.getEmission().getTexture().getSamplerId();
    //    CF::UniqueId src_id = c_sampler[idx]->getSourceImage();

    //    TextureMapInfo c_map_info;
    //    //memorize the texture map of this channel
    //    CF::TextureMapId c_map_id = 
    //        _c_fx_cmn.getEmission().getTexture().getTextureMapId();

    //    c_map_info.c_texture_map_id = c_map_id;

    //    LayerToTextureMapInfoMap::value_type v(GeometryProcessor::kUvLightMapName(),
    //                                           c_map_info);

    //    if (!c.layer_to_map_info.insert(v).second) {
    //        cout << "Error could not insert to map-id to layer name cache of "
    //             << "effect." << endl;
    //        return false;
    //    }

    //    //Lookup if we have seen this image, and what its promised path is
    //    //going to be
    //    BakerCache::ImageBakeCache::const_iterator it_img = 
    //        _baker->cache().images.find(src_id);
    //    if (it_img != _baker->cache().images.end()) {

    //        rtr_param->set_svalue(it_img->second.img_path);
    //        c.used_images.push_back(src_id);

    //    } else {
    //        cout << "Referenced emission texture: " << src_id.toAscii() 
    //             << " does not " 
    //             << "exist." << endl;

    //        cout << "Using 'stock_black.png' instead." << endl;
    //    }
    //    
    //}

    c.material_prototype = _rtr_material_prototype;

    BakerCache::EffectBakeCache::value_type v(_c_id, c);
    if (!_baker->cache().effects.insert(v).second) {
        cout << "Could not insert into effects cache." << endl;
        return false;
    }

    return true;
}

const CF::Sampler * EffectProcessor::find_sampler(const string& id) {

    const CS::Loader& loader = _baker->sax_loader();
    for (size_t i = 0; i<_c_fx_cmn.getSamplerPointerArray().getCount(); ++i) {
        const CF::Sampler * c_smpl = _c_fx_cmn.getSamplerPointerArray()[i];
        if (c_smpl->getOrigSid() == id)
            return c_smpl;
    }
    return NULL;
}

string EffectProcessor::get_string_for_color(const COLLADAFW::Color& c_color)
{

    std::ostringstream oss;
    int r = static_cast<int>(255 * c_color.getRed());
    int g = static_cast<int>(255 * c_color.getGreen());
    int b = static_cast<int>(255 * c_color.getBlue());

    oss << "rgb(" << r << " ," << g << ", " << b << ")";

    return oss.str();
}
