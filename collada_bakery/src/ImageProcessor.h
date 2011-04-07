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

#ifndef __CB_IMAGE_PROCESSOR_H
#define __CB_IMAGE_PROCESSOR_H

#include "Processor.h"

namespace ColladaBakery {

    class Baker;

    class ImageProcessor : public Processor {
    public:
        ImageProcessor(Baker* baker);

        struct BakeCache {
            //This is always a relative string to the destination directory
            //as set in config file.
            string img_path;
        };

        //Process checks if the file exists, if not it will not be added
        //to the baker cache. Consequently, material using this texture
        //will be aware of that.
        bool process(const CF::Object* cObject);

        //The post process runs very late and will copy images to the 
        //destination asset directory, if they were used (and if
        //copying textures is enabled)
        bool post_process();

    private:

        CF::UniqueId _c_id;
        string _src_file_path;
    };

    typedef boost::shared_ptr<ImageProcessor> ImageProcessorRef;
}

#endif //__CB_IMAGE_PROCESSOR_H