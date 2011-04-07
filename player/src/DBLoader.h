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

#ifndef DBLOADER_H
#define DBLOADER_H

#include "common.h"

//Note: windows.h which is included by ExtGL.h has a #define ERROR, this screws
//up compilation of kyoto lib, therefore we need to undef it. (same fr SYNCHRONIZE)

#undef ERROR
#undef SYNCHRONIZE

#ifdef _MSC_VER
#pragma warning( disable : 4244)
#pragma warning( disable : 4351)
#endif 

#include <kchashdb.h>

#include <boost/scoped_array.hpp>

namespace kc = kyotocabinet;

/**
 * This class loads a kyoto file-based hash database. The entries must be
 * valid rtr_format messages.
 * 
 * At the moment this class performs very simple tasks by loading one single
 * database that has been baked from a single COLLADA to rtr_format baking
 * process. This might be extended in the future where multiple scenes must
 * be managed (i.e. ID resolution will have to considered more carefull across
 * multiple document instances). Also, we might delay loading some resources,
 * and stream them at a later point. This, however, requires more complex 
 * balancing heuristics. For now, we just load one single scene at once.
 */
class DBLoader : boost::noncopyable
{
public:

    /**
     * Creates a new DB loader based on the path to a DB file. At the moment
     * this loads a single scene file.
     * Note that the path must be relative to the asset-directory which is
     * defined by the configuration property "asset_dir".
     */
    DBLoader(const string& path);
    ~DBLoader();

    /**
     * Initializes the database (opens connections, etc...). 
     */
    bool initialize();

    string startup_scene_id(); 

    template <typename T>
    void read(const string& key, boost::shared_ptr<T>& value_out);

private:
    bool _is_initialized;
    string _db_path;
    kc::HashDB _db;

};

typedef boost::scoped_array<char> DataPtr;

template <typename T>
void DBLoader::read(const string& key, boost::shared_ptr<T>& value_out) {

    if (!_is_initialized) {
        cerr << "Error: Database is not initialized." << endl;
        return;
    }

    //Note: we use the C string array retrieval method, since C++ string
    //retrieval requires copying in kyotocabinet lib.
    size_t val_size = 0;
    DataPtr data(_db.get(key.c_str(), key.size(), &val_size));
    if (!data) {
        cerr << "Key " << key << " does not exist." << endl;
        return;
    }

    boost::shared_ptr<T> obj_ref(new T());
    bool parse_success = obj_ref->ParseFromArray(data.get(), val_size);
    if (!parse_success) {
        cout << "Could not parse rtr object with type " << obj_ref->GetTypeName() 
             << "." << endl;
        return;
    }

    value_out = obj_ref;

    return;
}

#endif //DBLOADER_H