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

#ifndef COMMON_CONST_H
#define COMMON_CONST_H

/**
 * Defines a few string constants which might be used by other utilties.
 */

namespace rtr {

    inline const string& kStartupSceneID() {
        static const string s = "startup_scene";
        return s;
    }

    namespace Targets {

        //string constants for targeting with our animation system
        static const string& LOOKAT_POSITION_TARGET() {
            static const string target("/position");
            return target;
        }

        static const string& LOOKAT_POINT_OF_INTEREST_TARGET() {
            static const string target("/point_of_interest");
            return target;
        }

        static const string& LOOKAT_UP_TARGET() {
            static const string target("/up");
            return target;
        }

        static const string& ROTATE_AXIS_TARGET() {
            static const string target("/axis");
            return target;
        }

        static const string& ROTATE_ANGLE_TARGET() {
            static const string target("/angle");
            return target;
        }

        static const string& CAM_FOV_TARGET() {
            static const string target("/fov_value");
            return target;
        }

        static const string& LIGHT_MULTIPLIER_TARGET() {
            static const string target("/multiplier");
            return target;
        }

        static const string& LIGHT_INTENSITY_TARGET() {
            static const string target("/intensity");
            return target;
        }

    }

    static const string& ERROR_MATERIAL_NAME() {
        static const string name("error");
        return name;
    }

    static const string& DEFAULT_TEXTURES_PATH_PREFIX() {
        static const string s("@DEFAULT@");
        return s;
    }

    //Format: <pathname>"_nm"<optional_unique_id_suffix><extension>
    static const string& kNormalMapFormat() {
        static const string regex_pathname = "[[:graph:]_/\\\\]+";
        static const string regex_nm_suffix = "_nm";
        static const string regex_unique_suffix = "(_\\d+)?";
        static const string regex_extension = ".\\w+";

        static const string regex_pattern = ("^" + regex_pathname 
                                                    + regex_nm_suffix
                                                    + regex_unique_suffix 
                                                    + regex_extension
                                                    + "$");

        return regex_pattern;
    }



}

#endif //COMMON_CONST_H