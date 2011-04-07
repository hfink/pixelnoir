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

#ifndef __CB_PROCESSOR_H
#define __CB_PROCESSOR_H

#include "COLLADAFWObject.h"
#include "cbcommon.h"

namespace ColladaBakery {

    //fwd decl.
    class Baker;

    /**
     * This is the common base class for all processors of COLLADA elements.
     * Specific implementation have to implement the pure virtual interfaces
     * of this class.
     */
    class Processor : noncopyable, public boost::enable_shared_from_this<Processor> {
        friend class Baker;
    public:
        /**
         * Return the success of the process method. This is in-place processing
         * If the implementation is not able to process everything in place, 
         * it might register itself for postprocess in the Baker.
         */
        virtual bool process(const CF::Object* cObject) = 0;
        virtual bool post_process() { return true; }

    protected:

        Processor(Baker* baker);
        virtual ~Processor();

        Baker* _baker;
    };

    typedef boost::shared_ptr<Processor> ProcessorRef;
}

#endif //__CB_PROCESSOR_H