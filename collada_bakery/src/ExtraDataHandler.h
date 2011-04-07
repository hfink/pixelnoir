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

#ifndef __CB_EXTRA_DATA_HANDLER_H
#define __CB_EXTRA_DATA_HANDLER_H

#include <vector>

#include "COLLADASaxFWLIExtraDataCallbackHandler.h"
#include "GeneratedSaxParserTypes.h"
#include "cbcommon.h"

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

namespace GSP = GeneratedSaxParser;

namespace ColladaBakery {

    class Baker;

    class ExtraDataHandler : public COLLADASaxFWL::IExtraDataCallbackHandler {
    
    public:

        static const string& k3DSMaxProfile() { 
            static const string s("OpenCOLLADA3dsMax");
            return s;
        }

        static const string& kEmptyResult() { 
            static const string s("");
            return s;
        }

        ExtraDataHandler(Baker* baker);

        /**
         * Returns an attribute of an element under an extra tag.
         * Returns an empty string if no element with the specified
         * attribute exists.
         */
        const string& get_attribute(const CF::UniqueId& element,
                                    const string& profile,
                                    const string& path, 
                                    const string& att_name) const;

        /**
         * Returns the content data of a specific attribute.
         */
        const string& get_content(const CF::UniqueId& element,
                                  const string& profile,
                                  const string& path) const;

        //Interface implementations
        bool elementBegin( const GSP::ParserChar* elementName, 
                           const GSP::xmlChar** attributes );
        bool elementEnd(const GSP::ParserChar* elementName );
        bool textData(const GSP::ParserChar* text, size_t textLength);
        bool parseElement ( const GSP::ParserChar* profileName, 
                            const GSP::StringHash& elementHash, 
                            const CF::UniqueId& uniqueId );
    private:

        Baker * _baker;

        static const string& kCDataKey() {
            static const string s("__rtrcdata__");
            return s;
        }

        string path_string() const;

        //elementId (owner of extra tag), profile, path, attribute
        //Note that we use the special attribute __rtrcdata__
        //to specify content data of an element.
        typedef boost::tuple<CF::UniqueId, string, string, string> Key;
        typedef map<Key, string> DataMap;

        DataMap _data;

        string _current_profile;
        CF::UniqueId _current_element;

        vector<string> _current_path;

    };
}

#endif //__CB_EXTRA_DATA_HANDLER_H
