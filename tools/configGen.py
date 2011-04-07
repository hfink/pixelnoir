#!/usr/bin/python

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
from optparse import OptionParser
from xml.etree.ElementTree import ElementTree
import os.path

from Cheetah.Template import Template

script_dir = os.path.dirname(__file__) + os.sep

def parse_file(filename):
    tree = ElementTree()
    tree.parse(filename)
    
    #config_element = tree.find("config")
    classname = tree.getroot().attrib["name"]
    include = tree.findtext("include")
    
    enums = {}
    for enum in tree.findall("enums/enum"):
        elements = map(lambda f: f.attrib["name"], enum.getiterator("element"))
        enums[enum.attrib["name"]] = elements

    values = []
    for v_elem in tree.findall('values/value'):
        v = {}
        v['name'] = v_elem.attrib['name']
        v['type'] = v_elem.attrib['type']
        v['default'] = v_elem.attrib['default']
        v['comment'] = v_elem.text
        values.append(v)

    global_vars = []

    for g_elem in tree.findall('global'):
        global_vars.append(g_elem.attrib['name'])
    
    return {"classname":classname,
            "include":include,
            "enums":enums,
            "values":values,
            "filename":filename,
            "globals":global_vars}
                

parser = OptionParser("usage: %prog [options] input_file")
parser.add_option("-H", "--header_ext", dest="header_ext",
                  help="file extension of generated header file", metavar="EXT",
                  default="hh")
parser.add_option("-C", "--source_ext", dest="source_ext",
                  help="file extension of generated source file", metavar="EXT",
                  default="cc")
parser.add_option("-D", "--dir", dest="output_dir",
                  help="output directory", metavar="DIR",
                  default=".")

(options, args) = parser.parse_args()

if len(args) != 1:
    parser.error("incorrect number of arguments")

filename = args[0]

parsed_file = parse_file(filename)
classname = parsed_file["classname"]

template_namespace = {'classname' : classname,
                      'header_ext' : options.header_ext,
                      'include' : parsed_file['include'],
                      'enums' : parsed_file['enums'],
                      'fields' : parsed_file['values'],
                      'globals' : parsed_file['globals'],
                      'header_ext' : options.header_ext
                      }

header_template = Template(open(script_dir+'config_header_template.hh', 'r').read(),
                           template_namespace)
source_template = Template(open(script_dir+'config_source_template.cc', 'r').read(),
                           template_namespace)

header_file = open('%s/%s.%s' % (options.output_dir, 
                                 classname, 
                                 options.header_ext),
                   'w')

source_file = open('%s/%s.%s' % (options.output_dir, 
                                 classname, 
                                 options.source_ext),
                   'w')

header_file.write(str(header_template))
source_file.write(str(source_template))

