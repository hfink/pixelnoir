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

#include "ExtraDataHandler.h"

#include "Baker.h"

using namespace ColladaBakery;

ExtraDataHandler::ExtraDataHandler(Baker* baker) : 
    _baker(baker)
{
}

const string& ExtraDataHandler::get_attribute(const CF::UniqueId& element,
                                              const string& profile,
                                              const string& path, 
                                              const string& att_name) const
{
    Key k(element, profile, path, att_name);
    DataMap::const_iterator it = _data.find(k);
    return (it != _data.end() ? it->second : kEmptyResult());
}

const string& ExtraDataHandler::get_content(const CF::UniqueId& element,
                                            const string& profile,
                                            const string& path) const
{
    Key k(element, profile, path, kCDataKey());
    DataMap::const_iterator it = _data.find(k);
    return (it != _data.end() ? it->second : kEmptyResult());
}

string ExtraDataHandler::path_string() const {
    string path_string;
    vector<string>::const_iterator it;
    for (it = _current_path.begin(); 
         it != _current_path.end();
         ++it)
    {
        if (it != _current_path.begin())
            path_string += "/";

        path_string += *it;
    }

    return path_string;
}

bool ExtraDataHandler::elementBegin( const GSP::ParserChar* elementName, 
                                     const GSP::xmlChar** attributes ) 
{
    _current_path.push_back(string(elementName));
    string path = path_string();
    for (int i = 0; 
         (attributes != NULL) && //The array of arays
         (attributes[i] != NULL) && //The name of the attribute
         ((attributes[i+1] != NULL)); //The content of the attribute
         i+=2)
    {
        string attribute_name(attributes[i]);
        string attribute_data(attributes[i+1]);

        Key k(_current_element,
              _current_profile, 
              path,
              attribute_name);

        DataMap::value_type v(k, attribute_data);
        if (!_data.insert(v).second)
            cout << "Error: Could not insert attr into <extra> data map." << endl;
    }
    return true;
}

bool ExtraDataHandler::elementEnd(const GSP::ParserChar* elementName ) {
    
    _current_path.pop_back();

    return true;
}

bool ExtraDataHandler::textData(const GSP::ParserChar* text, size_t textLength) {

    bool is_all_space = true;
    for (size_t i = 0 ; i<textLength; ++i)
        is_all_space &= (isspace(text[i]) != 0);

    //We don't care to track contents which consits of white-spaces only
    if (is_all_space)
        return true;

    string cdata_str(text, textLength);

    string path = path_string();

    Key k(_current_element,
          _current_profile, 
          path,
          kCDataKey());

    DataMap::value_type v(k, cdata_str);
    if (!_data.insert(v).second)
        cout << "Error: Could not insert cdata into <extra> data map." << endl;

    return true;
}

bool ExtraDataHandler::parseElement ( const GSP::ParserChar* profileName, 
                                      const GSP::StringHash& elementHash, 
                                      const CF::UniqueId& uniqueId )
{

    //At the moment we only parse OpenCOLLADA 3dsmax extra profile.
    string prof(profileName);
    _current_profile = prof;
    _current_element = uniqueId;

    return prof == "OpenCOLLADA3dsMax";
}
