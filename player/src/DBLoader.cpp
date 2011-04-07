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

#include <kcfile.h>

#include "RtrPlayerConfig.h"
#include "common_const.h"

#include "rtr_format.pb.h"


DBLoader::DBLoader(const string& path) :
    _is_initialized(false)
{
    //setup the path
    _db_path =  path;
}

bool DBLoader::initialize() {
    
    if (_is_initialized) {
        cout << "Warning: DBLoader is already initialized. DBLoader won't be"
             << "re-initialized." << endl;
        return true;
    }

    kc::File::Status s;
    bool s_result = kc::File::status(_db_path, &s);

    if (!s_result) {
        cout << "Error: File " << _db_path << " does not exist." << endl;
        return false;
    } else if (s_result && s.isdir) {
        cout << "Error: File " << _db_path << " is a directory." << endl;
        return false;
    }

    //file exists, setup DB
    bool b = _db.open(_db_path, kc::HashDB::OREADER | kc::HashDB::ONOLOCK);
    if (!b) {
        cout << "Error: Failed to open db: " << _db.error().name() << endl;
        return false;
    }

    _is_initialized = true;

    return true;

}

DBLoader::~DBLoader() {

    //close the database
    bool b = _db.close();

    if (!b)
        cout << "Error closing database: " << _db.error().name() << endl;

}

string DBLoader::startup_scene_id() {
    string* startup_scene_id = _db.get(rtr::kStartupSceneID());

    if (startup_scene_id == NULL) {
        cout << "File " << _db_path << " does not contain a startup scene id"
             << "." << endl;
        return "";
    } else {
        string cpy = *startup_scene_id;
        delete startup_scene_id;
        return cpy;
    }
}
