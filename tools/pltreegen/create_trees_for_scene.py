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

# Main script for PL system scene generation

import os.path

from turtleinterpreter import TurtleInterpreter
from ternarytree import TernaryTree

from optparse import OptionParser

def parse_args():
    parser = OptionParser()
    parser.add_option("-i", "--incubator",
                      dest="incubator",
                      default="./input.rtr",
                      help="The incubator scene, i.e. the scene in which " +
                           "trees will be placed in. Has to be a *.rtr file.")
    parser.add_option("-o", "--output",
                      dest="output",
                      default="./output.rtr",
                      help="Output scene that we will write to.")
    parser.add_option("-c", "--compress",
                      dest="compress",
                      action="store_true",
                      help="If flag is set, output values will be compressed.")
    parser.add_option("-b", "--bake",
                      dest="bake",
                      action="store_true",
                      help="If flag is set, the turtle interpretation would " +
                           "rather bake all drawn segments into one geometry, " +
                           "the whole tree is one mesh, where otherwise each " +
                           "segment within the tree is just a node with its own " +
                           "transformation. Use the former for performance and the " +
                           "latter for benchmark the performance of large " +
                           "transformation hierarchies.")
    parser.add_option("-s", "--segment",
                      dest="segment",
                      default="./segment.rtr",
                      help="The scene containing the segment prototype, i.e. " +
                           " this is used during the turtle interpretation as " +
                           " the 'move forward and draw' command.")
    parser.add_option("-k", "--segmentkey",
                      dest="segmentkey",
                      default="segmentShape",
                      help="The key of the segment in the segment input file.")
    parser.add_option("-l", "--startup",
                      dest="startup",
                      default="startup_scene",
                      help="The ID of the startup key to look up.")
    parser.add_option("-t", "--types",
                      dest="types",
                      default="",
                      help="Space separated strings of L system types. " +
                           "Currently, these ternarytree_1 to ternarytree_4.")      
    
    return parser.parse_args()
    
(options, args) = parse_args()

# Check some parameters
def check_file(path):
    if not os.path.exists(path):
        raise RuntimeError("File "+ path + " does not exist.")

check_file(options.incubator)
check_file(options.segment)

interpreter = TurtleInterpreter(options.incubator,
                                options.output,
                                options.compress,
                                options.segment,
                                options.segmentkey,
                                options.startup,
                                1,
                                options.bake)

# Types we have available
generators = {}
generators["ternarytree_1"] = TernaryTree(94.74,
                                          132.63,
                                          18.95,
                                          1.109,
                                          1.732,
                                          [0, 0, -1],
                                          0.3)

generators["ternarytree_2"] = TernaryTree(137.5,
                                          137.5,
                                          18.95,
                                          1.109,
                                          1.732,
                                          [0, 0, -1],
                                          0.14)

generators["ternarytree_3"] = TernaryTree(112.50,
                                          157.5,
                                          22.5,
                                          0.9,
                                          1.732,
                                          [-0.02,0, -1],
                                          0.27)

generators["ternarytree_4"] = TernaryTree(180.00,
                                          252.00,
                                          36.00,
                                          1.07,
                                          1.732,
                                          #[-0.61,0.77,-0.19],
                                          [-0.61,-0.19,0.77],
                                          0.4)

# Get the types
types = options.types.split()
for type_str in types:
    if type_str not in generators:
        print("Have no generator of type: " + type_str)
        continue
    generator = generators[type_str]
    lstring = generator.produce(6)
    interpreter.dim_scale = generator.dim_scale
    interpreter.interpret(lstring,
                          type_str,
                          generator.T,
                          generator.ts,
                          0.2)

interpreter.write_and_close()
    
    
