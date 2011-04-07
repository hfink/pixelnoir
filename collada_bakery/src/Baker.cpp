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

#include "Baker.h"
#include "Processor.h"
#include "ColladaBakeryConfig.h"
#include "Utils.h"

#include "COLLADAFW.h"

#include "VisualSceneProcessor.h"
#include "GeometryProcessor.h"
#include "CameraProcessor.h"
#include "LightProcessor.h"
#include "AnimationBindingProcessor.h"
#include "AnimationProcessor.h"
#include "MaterialProcessor.h"
#include "EffectProcessor.h"
#include "ImageProcessor.h"

#include "common_const.h"

#include <kcfile.h>

typedef kc::File::Status KCStatus; 

using namespace ColladaBakery;

Baker::Baker(const string& filepath) :
    _baking_successful(true),
    _error_handler(NULL),
    _baker_writer(NULL),
    _sax_loader(NULL),
    _root(NULL),
    _extra_handler(NULL)

{
    _baking_successful = true;

    _error_handler = new SaxErrorHandler();
    _baker_writer = new BakerWriter(this);
    _sax_loader = new COLLADASaxFWL::Loader(_error_handler);
    _root = new CF::Root(_sax_loader, _baker_writer);
    _extra_handler = new ExtraDataHandler(this);

    if (!_sax_loader->registerExtraDataCallbackHandler(_extra_handler))
        cout << "Error: Could not register extra data handler." << endl;
    
    string uri_string = CB::URI::nativePathToUri(filepath);

    _uri = CB::URI(uri_string);

    string bake_dest = bakery_config.outdir() + "/" + 
                       _uri.getPathFileBase() + ".rtr";

    _bake_file_uri = CB::URI(bake_dest);
    CB::URI tmp(_bake_file_uri.getPathDir());

    _bake_file_dir = tmp.toNativePath();

    //we copy all used images into a folder called
    //BAKE_DST/COLLADA_FILE_NAME_images

    _img_dst_path = _uri.getPathFileBase() + "_images/";

}

Baker::~Baker() {

    finalize_transactions();

    if (_error_handler != NULL)
        delete _error_handler;
    if (_baker_writer != NULL)
        delete _baker_writer;
    if (_sax_loader != NULL)
        delete _sax_loader;
    if (_root != NULL)
        delete _root;
    if (_extra_handler != NULL)
        delete _extra_handler;
}

bool Baker::bake() {

    //Let's check if the COLLADA file actually exists
    KCStatus s;
    bool result = kc::File::status(_uri.toNativePath(), &s);
    if (!result) {
        cout << "File " << _uri.toNativePath() << " does not exist." << endl;
        return false;
    } else if (result && s.isdir) {
        cout << _uri.toNativePath() << " is a directory." << endl;
        return false;
    }

    //This triggers everything from in-place processing to post-processing

    bool b = open_db();
    if (!b)
        return false;

    bool document_success = _root->loadDocument(_uri.toNativePath());

    b = close_db();
    if (!b)
        return false;

    finalize_transactions();

    //Copy used images to their destination folder (only if baking was
    //successful).
    if (_baking_successful && !copy_images())
        return false;

    return (document_success && _baking_successful);
}

void Baker::register_for_postprocess(ProcessorRef processor, int order_weight) {

    //use the order weight as insert suggestion. If already occupied, increases
    //the weight. Note that std::map does not implement chaining, therefore 
    //we have to do this.
    PostProcessChain::value_type v(order_weight, processor);
    _post_process_chain.insert(v);

}

bool Baker::copy_images() {

    //make sure the image destination exists

    string img_dir = _bake_file_dir + _img_dst_path;
    KCStatus s;
    bool result = kc::File::status(img_dir, &s);
    if (result && !s.isdir) {
        cout << "Unexpected file " << img_dir << endl;
        cout << "This is expected to be a directory." << endl;
        return false;
    }

    if (!result) {
        bool b = kc::File::make_directory(img_dir);
        if (!b) {
            cout << "Could not create directory '" << img_dir << "'." << endl;
            return false;
        }
    }

    StringPair::const_iterator it;
    for (it = _images.begin(); it != _images.end(); ++it) {
        string src = it->first;
        string dst = _bake_file_dir + it->second;

        //TODO: we might copy large files more efficiently by copying 
        //subsequent blocks (or stream it)
        int64_t src_size = 0;
        char * src_data = kc::File::read_file(src, &src_size);

        if (src_data == NULL) {
            cout << "Error: Could not read file " << src << endl;
            return false;
        }

        bool b = kc::File::write_file(dst, src_data, src_size);

        delete[] src_data;

        if (!b) {
            cout << "Error: Could not write to file " << dst 
                 << endl;
            return false;
        }
        
        cout << "Copy: '" << src << "' -> '" << dst << "'." << endl;
    }

    return true;
}

const BakerCache& Baker::cache() const {
    return _cache;
}

BakerCache& Baker::cache() {
    return _cache;
}

void Baker::post_process() {

    PostProcessChain::iterator it; 
    for (it = _post_process_chain.begin();
         it != _post_process_chain.end();
         ++it)
    {
        if (!it->second->post_process()) {
            cerr << "Post-process stage failed." << endl;
            fail();
        }
    }

    _post_process_chain.clear();

    //we also want to set the startup scene
    if (_cache.startup_scene.isValid()) {
        //look it up
        BakerCache::VisualSceneBakeCache::const_iterator it = 
                                       _cache.scenes.find(_cache.startup_scene);
        if (it == _cache.scenes.end()) {
            cout << "Error: instantiated scene " 
                 << _cache.startup_scene.toAscii() 
                 << " was not found." << endl;
            fail();
        } else {
            bool b = _bake_db.set(rtr::kStartupSceneID(), it->second.id);
            if (!b) {
                cout << "Error: could not write key " << rtr::kStartupSceneID
                     << "." << endl;
                fail();
            }
        }
    } else {
        //just take the first scene in cache
        if (_cache.scenes.empty()) {
            cout << "Error: no scenes were loaded." << endl;
            fail();
        } else {
            string scene_id = _cache.scenes.begin()->second.id;

            bool b = _bake_db.set(rtr::kStartupSceneID(), scene_id);
            if (!b) {
                cout << "Error: could not write key " << rtr::kStartupSceneID
                     << "." << endl;
                fail();
            }
        }
    }

}

//string Baker::make_unique_id(const string& pref_id) {
//    return Utils::make_unique(pref_id, _cache.id_cache);
//}

bool Baker::open_db() {
    
    //check if the target file exists, if it does, rename it, and add
    //it properly to cache, if it does not just write
    string bake_file_path = 
        bakery_config.outdir() + 
        kc::File::PATHSTR + 
        _uri.getPathFileBase() + 
        ".rtr";

    //If the bake destination folder
    vector<string> path_components;
    CB::Utils::split(_bake_file_uri.getPathDir(), "/", path_components);
    
    string accum_path = "";
    vector<string>::const_iterator it;
    for (it = path_components.begin(); 
         it != path_components.end(); 
         ++it)
    {

        if (it != path_components.begin())
            accum_path += kc::File::PATHSTR;

        accum_path += *it;

        KCStatus status;
        bool result = kyotocabinet::File::status(accum_path, &status);
        if (!result) {
            //let's try to create this path
            bool b = kc::File::make_directory(accum_path);
            if (!b) {
                string abs = kc::File::absolute_path(accum_path);
                cerr << "Cannot create path " + abs + "." << endl;
                return false;
            }
        }

    }

    //at this point, the path of the bake target should be valid, and we can
    //open/overwrite the actual DB file

    //check if the target file exists
    KCStatus bake_file_status;
    bool bake_file_result = kc::File::status(bake_file_path, &bake_file_status);

    string old_file = "";
    if (bake_file_result && bake_file_status.isdir) {
        //our target is a directory
        cerr << "Cannot write file " << bake_file_path 
             << " as it is a directory.";
        return false;
    } else if (bake_file_result) {
        //seems to be a file, but already exists, rename it and memorize this
        string new_filename = bake_file_path+"__tmp";
        bool b = kc::File::rename(bake_file_path, new_filename);
        if (!b) {
            cerr << "Could not rename " << bake_file_path << " to "
                 << new_filename << "." << endl;
            return false;
        }
        old_file = new_filename;
    }

    //use compression
    if (bakery_config.db_compression()) {
        _bake_db.tune_options(kc::HashDB::TLINEAR | kc::HashDB::TCOMPRESS);
    } else {
        _bake_db.tune_options(kc::HashDB::TLINEAR);
    }
    if (!_bake_db.open( bake_file_path, 
                       kc::HashDB::OWRITER | 
                       kc::HashDB::OCREATE )) {
        cerr << "Could not open HashDB: " << _bake_db.error().name() << endl;
        return false;
    }

    //we have created the DB, let's memorize this as a transaction
    //At a later point we could roll-back all changes, if needed
    FileTransaction t(bake_file_path, old_file);
    _file_transactions.push_back(t);

    return true;
}

bool Baker::close_db() {
    //close database
    if (!_bake_db.close()) {
        cerr << "Could not close DB: " << _bake_db.error().name() << "." << endl;
        return false;
    }

    return true;
}

bool Baker::write_baked( const string& key, 
                         const google::protobuf::MessageLite * val ) {

    if (!val->SerializeToString(&_protobuf_cache)) {
        cerr << "Could not serialize protocol buffer of type " 
                << val->GetTypeName() << endl;
        return false;
    }

    //enter into DB
    if (!_bake_db.set(key, _protobuf_cache)) {
        cerr << "Could write into DB: " << _bake_db.error().name() 
                << "." << endl;
        return false;
    }

    return true;
}

void Baker::add_img(const string& src_path, const string& dst_name) {
    string dst_path = dst_name;
    _images.push_back(std::make_pair(src_path, dst_path));
}

void Baker::finalize_transactions() {
    
    FileTransactionList::iterator it_trans;
    for (it_trans = _file_transactions.begin(); 
         it_trans != _file_transactions.end();
         ++it_trans) 
    {

        if (_baking_successful) {
            //delete old files
            if (!it_trans->second.empty() && 
                !kc::File::remove(it_trans->second)) {
                cerr << "Could not delete file: " << it_trans->second << endl;
                fail();
            }
        } else {
            //delete new files, and restore old files
            if (!kc::File::remove(it_trans->first)) {
                cerr << "Could not delete file: " << it_trans->first << endl;
            }            
            if (!it_trans->second.empty() && 
                !kc::File::rename(it_trans->second, it_trans->first)) {
                cerr << "Could not rename file from  " 
                     << it_trans->second << " to " << it_trans->first << endl;
            }
        }

    }

    _file_transactions.clear();

}

Baker::BakerWriter::BakerWriter(Baker* baker) : 
    _baker(baker)
{}

void Baker::BakerWriter::cancel(const CF::String& errorMessage) {
    cout << "Canceled parsing process, because:" << errorMessage << endl;
    _baker->fail();
}

void Baker::BakerWriter::start() {
    cout << "Starting parsing process..." << endl;
}

void Baker::BakerWriter::finish() {

    cout << "Finished parsing process." << endl;

    if (_baker->status())
        _baker->post_process();

}

bool Baker::BakerWriter::writeGlobalAsset ( const CF::FileInfo* asset ) {
    return true;
}

bool Baker::BakerWriter::writeScene ( const CF::Scene* scene ) {

    _baker->cache().startup_scene = 
                     scene->getInstanceVisualScene()->getInstanciatedObjectId();

    return true; 
}

bool Baker::BakerWriter::writeVisualScene ( const CF::VisualScene* visualScene ) {
    VisualSceneProcessorRef ref(new VisualSceneProcessor(_baker));
    //process in place
    bool success = ref->process(visualScene);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeLibraryNodes ( const CF::LibraryNodes* libraryNodes ) {
return true; }

bool Baker::BakerWriter::writeGeometry ( const CF::Geometry* geometry ) {
    GeometryProcessorRef ref(new GeometryProcessor(_baker));
    //process in place
    bool success = ref->process(geometry);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeMaterial( const CF::Material* material ) {
    MaterialProcessorRef ref(new MaterialProcessor(_baker));
    //process in place
    bool success = ref->process(material);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeEffect( const CF::Effect* effect ) {
    EffectProcessorRef ref(new EffectProcessor(_baker));
    //process in place
    bool success = ref->process(effect);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeCamera( const CF::Camera* camera ) {
    CameraProcessorRef ref(new CameraProcessor(_baker));
    //process in place
    bool success = ref->process(camera);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeImage( const CF::Image* image ) {
    ImageProcessorRef ref(new ImageProcessor(_baker));
    //process in place
    bool success = ref->process(image);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeLight( const CF::Light* light ) {
    LightProcessorRef ref(new LightProcessor(_baker));
    //process in place
    bool success = ref->process(light);
    if (!success)
        _baker->fail();
    return success;
}

bool Baker::BakerWriter::writeAnimation( const CF::Animation* animation ) {
    AnimationProcessorRef ref(new AnimationProcessor(_baker));
    //process in place
    bool success = ref->process(animation);
    if (!success)
        _baker->fail();
    return success; 
}

bool Baker::BakerWriter::writeAnimationList( const CF::AnimationList* animationList ) {

    AnimationBindingProcessorRef ref(new AnimationBindingProcessor(_baker));
    //process in place
    bool success = ref->process(animationList);
    if (!success)
        _baker->fail();

    return success;
}

bool Baker::BakerWriter::writeSkinControllerData( const CF::SkinControllerData* skinControllerData ) {
return true; }

bool Baker::BakerWriter::writeController( const CF::Controller* controller ) {
return true; }

bool Baker::BakerWriter::writeFormulas( const CF::Formulas* formulas ) {
return true; }

bool Baker::BakerWriter::writeKinematicsScene( const CF::KinematicsScene* kinematicsScene ) {
return true; }