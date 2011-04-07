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

#ifndef __CB_BAKER_H
#define __CB_BAKER_H

#include "cbcommon.h"
#include "Types.h"
#include "BakerCache.h"
#include "GeometryProcessor.h"
#include "ExtraDataHandler.h"

#include "COLLADAFWIWriter.h"
#include "COLLADABUURI.h"
#include "SaxErrorHandler.h"
#include "COLLADASaxFWLLoader.h"
#include "COLLADAFWRoot.h"

#include "Utils.h"

#include <sstream>

#ifdef _MSC_VER
#pragma warning( disable : 4244)
#pragma warning( disable : 4351)
#endif 

namespace ColladaBakery {

    class Processor;
    
    /**
     * This baker class manages a couple of processors that convert certain 
     * aspect of COLLADA files (geometries, effects, etc...) into our defined
     * protocol buffer format. The converted representations of a particular
     * COLLADA element is stored in a key-value storage accessed and written
     * by the kyoto-cabinet library.
     *
     * The key to the binary blob of a converted COLLADA element is: 
     *
     *    "./RELATIVE_PATH_TO_FILE/" + "UniqueID_ASCII_REPRESENTATION".
     */
    class Baker : noncopyable {

    public:

        //Absolute file path. Caller is responsible for relative path 
        //resolution before.
        Baker(const string& filepath);
        virtual ~Baker();

        //triggers the reading process, bakes files, and returns the success
        //of the overall operation
        bool bake();

        void register_for_postprocess(ProcessorRef processor, int order_weight=0);

        //Note sure if we are going to need this, since we are able to derive
        //per-document unique IDs by CF facilities...
        //string make_unique_id(const string& pref_id);

        const BakerCache& cache() const;
        BakerCache& cache();

        bool write_baked( const string& key, 
                          const google::protobuf::MessageLite * val );

        //when this method is called, the baker is in the "fail" state, meaning
        //all subsequent operations might consider that state
        void fail() { _baking_successful = false; }
        bool status() const { return _baking_successful; } 

        template <typename T>
        inline string get_id(const T * c_element);

        /**
         * @param src_path The complete src_path to copy from.
         * @param dst_name The file name for the destination folder.
         */
        void add_img(const string& src_path, const string& dst_name);

        const CB::URI& uri() const { return _uri; }

        const ExtraDataHandler& extra_handler() { return *_extra_handler; }
        const COLLADASaxFWL::Loader& sax_loader() { return *_sax_loader; }

        const string& img_dst_path() const  { return _img_dst_path; }

    private:

        class BakerWriter;

        bool open_db();
        bool close_db();

        bool copy_images();

        /**
         * Nits things together, after having all elements processed. This is 
         * especially useful if can't convert thing "in-place" during the
         * write* callbacks of the IWriter interface.
         * In this method use your collected caches and connect the dots.
         */
        void post_process();

        void finalize_transactions();

        typedef std::list<std::pair<string, string> > StringPair;
        StringPair _images;

        std::ostringstream _log_stream;
        CB::URI _uri;
        string _bake_file_dir;
        CB::URI _bake_file_uri;

        string _img_dst_path;

        //<name of new file, name of old file (tmp rename file)
        typedef std::pair<string, string> FileTransaction;
        typedef list<FileTransaction> FileTransactionList;

        FileTransactionList _file_transactions;

        //Note how this list also prevents the processors from being destroyed
        //as they need postprocessing.
        typedef std::multimap<int, ProcessorRef > PostProcessChain;
        PostProcessChain _post_process_chain;

        BakerCache _cache;

        //a cache for protobuf serialization
        mutable string _protobuf_cache;

        bool _baking_successful;
        SaxErrorHandler* _error_handler;
        BakerWriter* _baker_writer;
        COLLADASaxFWL::Loader* _sax_loader;
        CF::Root* _root;
        ExtraDataHandler* _extra_handler;

        kc::HashDB _bake_db;
    };

    /**
     * Private class implementing the IWriter interface. This class will be
     * be fed with data that has been read by the SAX importer.
     */
    class Baker::BakerWriter : public CF::IWriter, noncopyable {

    public:

        BakerWriter(Baker * baker);
        virtual ~BakerWriter() {}
        /**
            * This method will be called when the loader encountered an error and
            * we should roll back everything that we have converted. In our case, 
            * we will delete all converted data blobs that we have written so far.
            */
        virtual void cancel(const CF::String& errorMessage);

        /**
            * Start writing out, preparing data.
            */
        virtual void start();

        /**
            * Will be called when we are done. No more writes are called afterwards
            */
        virtual void finish();

        /**
            * In the following we have all callbacks that will be called for
            * writing individual COLLADA elements, i.e. convert them into our
            * data structure. We will return true for successfull conversion, 
            * false otherwise.
            */

        virtual bool writeGlobalAsset ( const CF::FileInfo* asset );
        virtual bool writeScene ( const CF::Scene* scene );
        virtual bool writeVisualScene ( const CF::VisualScene* visualScene );
        virtual bool writeLibraryNodes ( const CF::LibraryNodes* libraryNodes );
        virtual bool writeGeometry ( const CF::Geometry* geometry );
        virtual bool writeMaterial( const CF::Material* material );
        virtual bool writeEffect( const CF::Effect* effect );
        virtual bool writeCamera( const CF::Camera* camera );
        virtual bool writeImage( const CF::Image* image );
        virtual bool writeLight( const CF::Light* light );
        virtual bool writeAnimation( const CF::Animation* animation );
        virtual bool writeAnimationList( const CF::AnimationList* animationList );
        virtual bool writeSkinControllerData( const CF::SkinControllerData* skinControllerData );
        virtual bool writeController( const CF::Controller* controller );
        virtual bool writeFormulas( const CF::Formulas* formulas );
        virtual bool writeKinematicsScene( const CF::KinematicsScene* kinematicsScene );
    private:
        Baker* _baker;
    };

    template <typename T>
    string Baker::get_id(const T * c_element) {
        return Utils::get_id(c_element, _cache.id_cache);
    }

}

#endif //__CB_BAKER_H
