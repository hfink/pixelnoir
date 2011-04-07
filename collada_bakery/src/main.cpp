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

#include <iostream>
#include "cbcommon.h"

#include "COLLADASaxFWLLoader.h"
#include "COLLADAFWRoot.h"
#include "COLLADABUURI.h"

#include "SaxErrorHandler.h"
#include "Baker.h"

#include "ColladaBakeryConfig.h"

#include "rtr_format.pb.h"

//we'll use some handy file utility routines of kyoto lib
#include "kcfile.h"

namespace kc = kyotocabinet;

using namespace ColladaBakery;

int main(int argc, char* argv[]) {

#ifdef ENABLE_WIN_MEMORY_LEAK_DETECTION
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

    string arg0(argv[0]);

    CB::URI arg0_uri(arg0);

    string cwd = kc::File::get_current_directory();

    //parse options
    string config_filename = "bakery_config.txt";

    bool lf_result = ColladaBakeryConfig::load_file(config_filename, bakery_config);
    if (!lf_result)
        std::cerr << "Could not load config file " << config_filename << std::endl;

    argc = bakery_config.parse_args(argc, argv);

    if (bakery_config.save_options() == true) {
        bakery_config.set_save_options(false);
        // Save config
        ColladaBakeryConfig::save_file(config_filename, bakery_config);
    }

    string input = bakery_config.input();

    if (input.empty()) {
        cout << "Nothing to process, use --input to specify input files." << endl;
    } else {
        vector<string> input_files;
        CB::Utils::split(input, " ", input_files);
        vector<string>::const_iterator it;
        
        for ( it = input_files.begin(); 
              it != input_files.end();
              ++it)
        {

            //convert to absolute path
            CB::URI input_uri(*it);
            bool is_relative = input_uri.getScheme().empty() && 
                               input_uri.getAuthority().empty();

            string input_native = input_uri.toNativePath();

            string filepath = input_native;
            if (is_relative) {
                filepath = cwd + kc::File::PATHSTR + input_native;
            }

            Baker baker(filepath);

            cout << "Baking COLLADA file '" << filepath << "' to '"
                 << bakery_config.outdir() + kc::File::PATHSTR 
                 << input_uri.getPathFileBase()
                 << ".rtr"
                 << "'."
                 << endl
                 << endl;

            bool result = baker.bake();

            if (result)
                cout << "   -> Success!" << endl;
            else
                cout << "   -> Failed!" << endl;
        }
    }

    //cout << endl << "Hit enter to exit." << endl;
    //std::cin.get();

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
