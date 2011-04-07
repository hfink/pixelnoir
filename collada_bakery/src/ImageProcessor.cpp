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

#include "ImageProcessor.h"

#include "Baker.h"

//That comes in handy for file-routines
#include "kcfile.h"

#include "COLLADAFWImage.h"

#include <sstream>

#include "ColladaBakeryConfig.h"

using namespace ColladaBakery;

ImageProcessor::ImageProcessor(Baker* baker) : 
Processor(baker)
{
}

bool ImageProcessor::process(const CF::Object* c_obj) {

    const CF::Image * c_img = static_cast<const CF::Image*>( c_obj );

    if (c_img->getSourceType() == CF::Image::SOURCE_TYPE_URI) {

        //OpenCOLLADA API is a little confusing in here.
        //However, the following URI obviously holds the image source.
        CB::URI c_img_uri = c_img->getImageURI();

        bool is_relative = c_img_uri.getScheme().empty() && 
                           c_img_uri.getAuthority().empty();

        string c_img_uri_path = c_img_uri.toNativePath();

        string path_to_check = c_img_uri_path;
        if (is_relative) {
            //TODO: Take care of optional base attribute of the COLLADA asset
            //attribute.
            string dir = _baker->uri().getPathDir();
            CB::URI dir_uri(dir);
            path_to_check = dir_uri.toNativePath() + kc::File::PATHSTR + 
                             c_img_uri_path;

        }

        //Note: The effect (and material) processor will know if an image
        //actually exists, if the image's uniqueId is not present in the bake
        //cache during their respective post-process stages.
        kc::File::Status s;
        bool result = kc::File::status(path_to_check, &s);
        if (!result || s.isdir) {
            cout << "Image '" << path_to_check << "' does not exist." << endl;
            cout << "Effects referencing this image will use default textures"
                 << " instead." << endl;
            return true;
        }

        _src_file_path = path_to_check;
        std::ostringstream oss;

        //The file name might have an ugly suffix, but that prevents any 
        //possible filename clashes
        oss << c_img_uri.getPathFileBase() << "_"
            << c_img->getUniqueId().getObjectId() << "."
            << c_img_uri.getPathExtension();

        string dst_file_path = _baker->img_dst_path() + oss.str();
                         
        //Image exists, mark in cache, and register for post-processing
        
        //Note: per convenience we always use "/" path delimiters. 
        //With relative paths only, they should work on Windows, too

        BakeCache c;
        c.img_path = dst_file_path;
        
        BakerCache::ImageBakeCache::value_type v(c_img->getUniqueId(), c);
        if (!_baker->cache().images.insert(v).second) {
            cout << "Program Error: Cannot insert into bake cache." << endl;
            //TODO: this happen, when opencollada export two image elements
            //with equal unique ids -> bug in opencollada exporter!!
            //return false;
        }

        _c_id = c_img->getUniqueId();

        //also register for post processing which will actually copy the
        //files (if they were used to their destination)
        //Postprocess has to run AFTER effect post-process
        _baker->register_for_postprocess(shared_from_this(), 5);

    } else {
        cout << "Warning: Import of <image> element is currently only "
             << "supported using the URI source type. Skipping import of '"
             << c_img->getOriginalId() << "'." << endl;
    }

    return true;
}

bool ImageProcessor::post_process() {

    BakerCache::UniqueIdSet::const_iterator it = 
                                       _baker->cache().used_images.find(_c_id);

    if (it != _baker->cache().used_images.end()) {

        //This image instance was used, let the baker know to copy this image
        //to our destination directory

        //We re-read our own cache, as the effect processor might have renamed
        //the file to fit a particular naming convention (e.g. _nm for normal
        //maps).
        string dst = _baker->cache().images.at(_c_id).img_path;
        _baker->add_img(_src_file_path, dst);

    } else {
        cout << "Image '" << _src_file_path << "' was not referenced by any " 
             << "active effects. Skipping copy operation." << endl;
    }

    return true;
}