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

#include "MaterialManager.h"

#include "Texture.h"
#include "Image.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "rtr_format.pb.h"
#include "RtrPlayerConfig.h"

#include "DBLoader.h"

#include <boost/regex.hpp>
#include "common_const.h"

using namespace rtr_format;

TextureRef TextureManager::load_texture(const string& file_name) 
{
    string path;

    const boost::regex replace_ptrn(rtr::DEFAULT_TEXTURES_PATH_PREFIX());
    const boost::regex search_ptrn(rtr::DEFAULT_TEXTURES_PATH_PREFIX() + ".*");
    const boost::regex rgb_pattern("rgb\\((\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\)");

    boost::match_results<std::string::const_iterator> match;

    if (boost::regex_match(file_name, match, rgb_pattern)) {
        int r,g,b;

        from_string<int>(match[1], r);
        from_string<int>(match[2], g);
        from_string<int>(match[3], b);

        Image image(2, 1, 1, 0,
                    GL_RGB, GL_FLOAT, sizeof(vec3));

        vec3* data = (vec3*)image.data();
        *data = vec3(r/255.0, g/255.0, b/255.0);

        TextureRef texture(new Texture(image));

        return texture;
    }

    if (boost::regex_match(file_name,search_ptrn)) {
        //This is a special refering to one of the stock-textures
        //we insert the configured path to stock textures instead of the 
        //prefix-by-convention
        path = boost::regex_replace( file_name, 
                                        replace_ptrn, 
                                        config.texture_dir() );
    } else {
        //This is just the normal path relative to the asset file
        std::string input_file = config.input();
        std::string safe_path = kc::File::absolute_path(input_file);

        if (safe_path.empty()) {
            cerr << "Error: could not retrieve absolute path of '" 
                 << input_file << "'." << std::endl;
        }

        std::size_t pos = safe_path.find_last_of(kc::File::PATHSTR);

        string load_path;

        if (pos != std::string::npos) {
            load_path = safe_path.substr(0, pos);
        }

        path = load_path + "/" + file_name;
    }

    if (!file_exists(path)) {
        cerr << "Could not load texture '" << path << "'. "
             << "Loading default texture instead." << endl;
        path = config.texture_dir() + "/stock_default.png";

        if (!file_exists(path)) {
            cout << "Default texture '" << path << "' is missing! "
                 << "Bailing out!" << endl;
            return TextureRef();
        }
    }
    
    Image image(path);
    TextureRef texture;
    
    if (image.data() != NULL) {
        texture = TextureRef(new Texture(image));
    }

    return texture;
}

TextureRef TextureManager::get_texture(const string& file_name)
{
    if (_texture_map.count(file_name) > 0) {
        
        weak_ptr<Texture>& tex = _texture_map.at(file_name);

        if (!tex.expired()) {
            return tex.lock();
        }
    }

    TextureRef tex = load_texture(file_name);

    if (tex)
        _texture_map[file_name] = weak_ptr<Texture>(tex);

    return tex;
}

void TextureManager::clear()
{
    _texture_map.clear();
}

void TextureManager::purge()
{
    map<string, weak_ptr<Texture> >::iterator to_erase;
    map<string, weak_ptr<Texture> >::iterator it = _texture_map.begin();
    
    while (it != _texture_map.end()) {
        if (it->second.expired()) {
            to_erase = it;
            ++it;
            _texture_map.erase(to_erase);
        } else {
            ++it;
        }
    }
}

MaterialInstance::MaterialInstance(int material_id, int instance_id,
                                   UniformBuffer* params,
                                   int texture_cnt) :
    _instance_id(instance_id),
    _material_id(material_id),
    _params(params),
    _textures(texture_cnt)
{

}

MaterialInstance::~MaterialInstance()
{
    delete _params;
}

void MaterialInstance::reset(int material_id, int instance_id,
                             UniformBuffer* params, int texture_cnt)
{
    _textures.clear();
    delete _params;

    _material_id = material_id;
    _instance_id = instance_id;
    _params = params;
    _textures.resize(texture_cnt);
}

void MaterialInstance::set_texture_param(int index,
                                         const string& param_name, 
                                         TextureRef texture)
{
    _textures[index].name = param_name;
    _textures[index].tex = texture;
}

void MaterialInstance::bind(Shader& shader)
{
    _params->send_to_GPU();
    _params->bind();
    shader.set_uniform_block("Material", *_params);
    for(size_t i = 0; i < _textures.size(); ++i) {
        _textures[i].tex->bind();
        shader.set_uniform(_textures[i].name, *(_textures[i].tex));
    }
}

void MaterialInstance::unbind()
{
    _params->unbind();
    for(size_t i = 0; i < _textures.size(); ++i) {
        _textures[i].tex->unbind();
    }
}

MaterialManager::~MaterialManager()
{
    for(size_t i = 0; i < _shader_matrix.size(); ++i) {
        delete _shader_matrix[i];
    }
}

int MaterialManager::add_shader_program(const string& shader_program_name) 
{
    assert(_materials.size() == 0);

    int id = _shader_programs.size();
    _shader_programs.push_back(shader_program_name);
    return id;
}

bool MaterialManager::add_material(const string& material_name)
{
    bool valid = true;
    int id = _materials.size();

    _materials[material_name] = id;

    for (size_t i = 0; i < _shader_programs.size(); ++i) {
        Shader* shader = new Shader(_shader_programs[i], material_name);

        if (!shader->is_valid()) {
            cerr << "Shader combination " 
                 << _shader_programs[i] << "@" << material_name
                 << " does not compile." << endl;
            valid = false;
            _valid = false;
        }

        _shader_matrix.push_back(shader);
    }

    return valid;
}

MaterialInstanceRef MaterialManager::get_instance(const Material& material)
{
    if (_material_instances.count(material.id()) > 0 &&
        !_material_instances[material.id()].expired()) {
        return _material_instances[material.id()].lock();
    } else {
        return add_instance(material);
    }
}

MaterialInstanceRef MaterialManager::error_material(const string& name) {
    return add_error_material(name, MaterialInstanceRef());
}


MaterialInstanceRef MaterialManager::add_error_material(const string& name,
                                                        MaterialInstanceRef inst) {

    rtr_format::Material dummy_error_mat;
    dummy_error_mat.set_id(name);
    dummy_error_mat.set_shader("Constant");

    rtr_format::Parameter* rtr_param = dummy_error_mat.add_parameter();
    rtr_param->set_name("color");
    rtr_param->set_type(rtr_format::VEC3);
    rtr_param->add_fvalue(1);
    rtr_param->add_fvalue(0);
    rtr_param->add_fvalue(0);

    return add_instance(dummy_error_mat, inst);
}

MaterialInstanceRef MaterialManager::add_instance(const Material& material,
                                                  MaterialInstanceRef inst)
{
    if (_materials.size() < 1) {
        add_material("Constant");
    }

    if (_materials.count(material.shader()) < 1) {
        if (!add_material(material.shader())) {
            MaterialInstanceRef error = add_error_material(material.id(), inst);
            return error;
        }
    }

    int mat_id = _materials[material.shader()];

    size_t pos = mat_id * _shader_programs.size();
    Shader* shader = (_shader_matrix[pos]);

    if (!shader->is_valid()) {
        MaterialInstanceRef error = add_error_material(material.id(), inst);
        return error;
    }

    UniformBuffer* ubo = new UniformBuffer(*shader, "Material");

    list<rtr_format::Parameter> texture_params;

    for (int i = 0; i < material.parameter_size(); ++i) {
        const rtr_format::Parameter& p = material.parameter(i);
        //If we don't have this parameter either as a free standing
        //uniform, or as part of the Material uniform block, we skip
        //it and print a warning.
        if (! (ubo->has_entry(p.name()) || shader->has_uniform(p.name()) ) ) {
            cout << "Warning (" << material.id() << "): Shader '"
                 << material.shader() << "' has no parameter named '"
                 << p.name() << "'. Skipping parameter." << endl;
            continue;
        }
        switch (p.type()) {
        case rtr_format::TEXTURE: 
            texture_params.push_back(p);
            break;
        case rtr_format::INT:
            assert(p.ivalue_size() == 1);
            ubo->set(p.name(), p.ivalue(0));
            break;
        case rtr_format::IVEC2:
            assert(p.ivalue_size() == 2);
            ubo->set(p.name(), ivec2(p.ivalue(0), p.ivalue(1)));
            break;
        case rtr_format::IVEC3:
            assert(p.ivalue_size() == 3);
            ubo->set(p.name(), ivec3(p.ivalue(0), p.ivalue(1), p.ivalue(2)));
            break;
        case rtr_format::IVEC4:
            assert(p.ivalue_size() == 4);
            ubo->set(p.name(), ivec4(p.ivalue(0), p.ivalue(1), p.ivalue(2), p.ivalue(3)));
            break;
        case rtr_format::FLOAT:
            assert(p.fvalue_size() == 1);
            ubo->set(p.name(), p.fvalue(0));
            break;
        case rtr_format::VEC2:
            assert(p.fvalue_size() == 2);
            ubo->set(p.name(), vec2(p.fvalue(0), p.fvalue(1)));
            break;
        case rtr_format::VEC3:
            assert(p.fvalue_size() == 3);
            ubo->set(p.name(), vec3(p.fvalue(0), p.fvalue(1), p.fvalue(2)));
            break;
        case rtr_format::VEC4:
            assert(p.fvalue_size() == 4);
            ubo->set(p.name(), vec4(p.fvalue(0), p.fvalue(1), p.fvalue(2), p.fvalue(3)));
            break;
        case rtr_format::MAT2:
            assert(p.fvalue_size() == 4);
            ubo->set(p.name(), mat2(p.fvalue(0), p.fvalue(1),
                                    p.fvalue(2), p.fvalue(3)));
            break;
        case rtr_format::MAT3:
            assert(p.fvalue_size() == 9);
            ubo->set(p.name(), mat3(p.fvalue(0), p.fvalue(1), p.fvalue(2), 
                                    p.fvalue(3), p.fvalue(4), p.fvalue(5),
                                    p.fvalue(6), p.fvalue(7), p.fvalue(8)));
            break;
        case rtr_format::MAT4:
            assert(p.fvalue_size() == 16);
            ubo->set(p.name(), mat4(p.fvalue(0), p.fvalue(1), p.fvalue(2), p.fvalue(3), 
                                    p.fvalue(4), p.fvalue(5), p.fvalue(6), p.fvalue(7), 
                                    p.fvalue(8), p.fvalue(9), p.fvalue(10),p.fvalue(11),
                                    p.fvalue(12),p.fvalue(13),p.fvalue(14),p.fvalue(15)));
            break;
        case rtr_format::SPECIAL:
            break;
        }
    }

    MaterialInstanceRef instance;

    if (!inst) {
        instance = 
            MaterialInstanceRef(new MaterialInstance(mat_id, 
                                                     _material_instances.size(),
                                                     ubo, texture_params.size()));
    } else {
        inst->reset(mat_id, 
                    _material_instances.size(), 
                    ubo, texture_params.size());
        instance = inst;
    }

    int tex_i = 0;
    list<rtr_format::Parameter>::const_iterator it_tex;
    for ( it_tex = texture_params.begin(); 
          it_tex != texture_params.end();
          ++it_tex)
    {
        if (!it_tex->has_svalue()) {
            cout << "Error: Texture parameter has not string value set." 
                 << endl;
            continue;
        }

        TextureRef tex = _texture_manager.get_texture(it_tex->svalue());

        if (tex) {
            instance->set_texture_param(tex_i, it_tex->name(), tex);
        }

        ++tex_i;
    }

    _material_instances[material.id()] = instance;
    _instance_map.push_back(instance->material_id());

    // cout << instance->instance_id() 
    //      << " " << instance->material_id() 
    //      << " " << _instance_map.size()-1 
    //      << " " << get_material_id(instance->instance_id()) << endl;

    return instance;
}

Shader& MaterialManager::get_shader(int shader_program_id,
                                    int material_id)
{
    int pos = material_id * _shader_programs.size() + shader_program_id;

    return *(_shader_matrix.at(pos));
}

void MaterialManager::reload(DBLoader* db_loader)
{
    for (size_t i = 0; i < _shader_matrix.size(); ++i) {
        delete _shader_matrix[i];
    }
    
    _shader_matrix.clear();
    _materials.clear();
    _shader_matrix.clear();
    _texture_manager.clear();
    
    map<string, weak_ptr<MaterialInstance> > tmp;

    _material_instances.swap(tmp);
    _instance_map.clear();

    map<string, weak_ptr<MaterialInstance> >::iterator i = tmp.begin();
    for (;i != tmp.end(); ++i) {
        if (i->second.expired()) {
            continue;
        }

        const string& name = i->first;

        shared_ptr<rtr_format::Material> material(new rtr_format::Material());

        db_loader->read(name, material);

        add_instance(*material, i->second.lock());
    }

    tmp.clear();
}
