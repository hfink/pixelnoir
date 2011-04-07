# Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
#                    Thomas Weber <weber (dot) t (at) gmx (dot) at>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import sys
import shutil
import os.path
from optparse import OptionParser

parser = OptionParser("usage: %prog [options] destination")
parser.add_option("-b", "--bakery",
                  dest="include_bakery",
                  action="store_true",
                  help="If set, the ColladaBakery will be included.")

(options, dests) = parser.parse_args()

if not dests:
    raise RuntimeError("No argument found. The first argument " +
                        "has to specify the target folder")

dest_folder = dests[0]

if not os.path.exists(dest_folder):
    os.makedirs(dest_folder)

# Copy files

files_to_copy = ['player/assets',
                 'player/material_shaders',
                 'player/shaders',
                 'player/audio',
                 'player/textures',
                 'external/devil/dll/DevIL.dll',
                 'external/devil/dll/ILUT.dll',
                 'external/devil/dll/ILU.dll',
                 'readme.txt',
                 'build/bin/Release/player.exe',
                 'external/libsfml/dll/openal32.dll',
                 'external/libsfml/dll/libsndfile-1.dll',
                 'player/player_config.txt']

if options.include_bakery:
    files_to_copy.append('tools/rebake_all.py')
    files_to_copy.append('tools/material_bakery.py')
    files_to_copy.append('materials')
    files_to_copy.append('ColladaBakery/assets_collada')
    files_to_copy.append('ColladaBakery/bakery_config.txt')
    files_to_copy.append('ColladaBakery/build/Release/ColladaBakery.exe')
    files_to_copy.append('Debug/generated/rtr_format_pb2.py')

for f in files_to_copy:
    if os.path.isdir(f):
        (head, tail) = os.path.split(f)
        target_tree = dest_folder + os.sep + tail
        if os.path.exists(target_tree):
            shutil.rmtree(target_tree)
        shutil.copytree(f, dest_folder + os.sep + tail)
    elif os.path.isfile(f):
        shutil.copy(f, dest_folder)
    else:
        print("Could not find file: " + f)


