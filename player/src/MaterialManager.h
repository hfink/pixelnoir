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

#ifndef MATERIALMANAGER_H
#define MATERIALMANAGER_H

#include "common.h"

namespace rtr_format
{
    class Material;
}

class Texture;
class Shader;
class UniformBuffer;
class DBLoader;

typedef shared_ptr<Texture> TextureRef;

class TextureManager {

    map<string, weak_ptr<Texture> > _texture_map;
    
    TextureRef load_texture(const string& file_name);

    public:
    TextureRef get_texture(const string& file_name);

    void clear();
    void purge();
};

class MaterialInstance;
typedef shared_ptr<MaterialInstance> MaterialInstanceRef;

class MaterialManager {

    public:

    MaterialManager() : _valid(true) {};
    virtual ~MaterialManager();

    // You have to call this before calling get_instance()!
    int add_shader_program(const string& shader_program_name);

    MaterialInstanceRef error_material(const string& name);

    MaterialInstanceRef get_instance(const rtr_format::Material& material);
    int material_count() { return _materials.size(); }
    int instance_count() { return _instance_map.size(); }

    int get_material_id(int instance_id) { return _instance_map[instance_id]; }

    Shader& get_shader(int shader_program_id, int material_id);

    bool is_valid() { return _valid; }

    void reload(DBLoader* _db_loader);

    private:

    bool _valid;

    MaterialInstanceRef add_instance(const rtr_format::Material& material,
                                     MaterialInstanceRef inst = MaterialInstanceRef());
    bool add_material(const string& material_name);

    MaterialInstanceRef add_error_material(const string& name,
                                           MaterialInstanceRef inst);

    TextureManager _texture_manager;
    map<string, weak_ptr<MaterialInstance> > _material_instances;
    vector<int> _instance_map;
    map<string, int> _materials;
    vector<string> _shader_programs;
    vector<Shader*> _shader_matrix;
};


class MaterialInstance {

    friend class MaterialManager;

    struct TextureParam {
        string name;
        TextureRef tex;
    };

    int _instance_id;
    int _material_id;
    UniformBuffer* _params;
    vector<TextureParam> _textures;

    MaterialInstance(int material_id, int instance_id,
                     UniformBuffer* params, int texture_cnt);
    void set_texture_param(int index, 
                           const string& param_name, TextureRef texture);

    public:

    ~MaterialInstance();

    int material_id() { return _material_id; }
    int instance_id() { return _instance_id; }
    void bind(Shader& shader);
    void unbind();

    void reset(int material_id, int instance_id,
               UniformBuffer* params, int texture_cnt);
};

#endif
