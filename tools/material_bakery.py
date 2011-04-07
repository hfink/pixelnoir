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
import os.path
from optparse import OptionParser
from xml.etree.ElementTree import ElementTree

sys.path.append(sys.path[0] + os.sep+".."+os.sep+"Debug"+os.sep+"generated")

import rtr_format_pb2
from kyotocabinet import *

def parse_file(filename):
    tree = ElementTree()
    tree.parse(filename)

    materials = []

    for m_elem in tree.findall('material'):
        id = m_elem.attrib['id']
        shader = m_elem.attrib['shader']

        params = []

        for p_elem in m_elem.findall('param'):
            pname = p_elem.attrib['name']
            type_name = p_elem.attrib['type']
            value = p_elem.attrib['value']
            
            params.append((pname,type_name,value))
            
        materials.append((id, shader, params))

    return materials

def print_material(material):
    id, shader, params = material

    print('%s (%s):' % (id, shader))
    
    for p in params:
        name, type_name, value = p

        print('\t%s (%s) = "%s"' % (id, type_name, value))

parser = OptionParser("usage: %prog [options] file(s)")
parser.add_option('-o', '--output', dest='output_file',
                  help='File name of output RTR file.', metavar='FILE')

(options, files) = parser.parse_args()

if not os.path.exists(options.output_file):
    raise RuntimeError("File " + options.output_file + " does not exist.")

type_conv = { "float" : rtr_format_pb2.FLOAT,
              "texture2D" : rtr_format_pb2.TEXTURE,
              "int" : rtr_format_pb2.INT,
              "vec2" : rtr_format_pb2.VEC2,
              "vec3" : rtr_format_pb2.VEC3,
              "vec4" : rtr_format_pb2.VEC4,
              "ivec2" : rtr_format_pb2.IVEC2,
              "ivec3" : rtr_format_pb2.IVEC3,
              "ivec4" : rtr_format_pb2.IVEC4,
              "mat2" : rtr_format_pb2.MAT2,
              "mat3" : rtr_format_pb2.MAT3,
              "mat4" : rtr_format_pb2.MAT4,
              "special" : rtr_format_pb2.SPECIAL }

# Open DB
db = DB()
if not db.open(options.output_file+"#type=kch", DB.OWRITER):
    raise RuntimeError("Cannot open DB: " + str(db.error()))

for f in files:
    materials = parse_file(f)
    
    for m in materials:
        id, shader, params = m

        rtr_mat = rtr_format_pb2.Material()
        data = db.get(id)
        if not data:
            print("Material " + id + " does not exist in DB.")
            print("This material will be added to the library.")
            rtr_mat.id = id
        else:
            rtr_mat.ParseFromString(data)

        # set the shader
        rtr_mat.shader = shader

        for p in params:

            name, type_name, value = p

            # Check if we have a parameter like this already
            # TODO: how to search more elegantly?
            # TODO: what
            idx = 0
            for rtr_p in rtr_mat.parameter:
                if rtr_p.name == name:
                    # Remove the parameter
                    del rtr_mat.parameter[idx]
                    break
                idx = idx +1

            # Add a new parameter
            new_param = rtr_mat.parameter.add()
            new_param.name = name

            # TODO: make this more elegant by using dicationaries
            # with function objects
            new_param.type = type_conv[type_name]
            if type_name == "float" or \
               type_name == "vec2" or \
               type_name == "vec3" or \
               type_name == "vec4" or \
               type_name == "mat2" or \
               type_name == "mat3" or \
               type_name == "mat4":
                farray = [float(s) for s in value.split()] 
                for f in farray:
                    new_param.fvalue.append(f)

            if type_name == "int" or \
               type_name == "ivec2" or \
               type_name == "ivec3" or \
               type_name == "ivec4":
                iarray = [int(s) for s in value.split()] 
                for i in iarray:
                    new_param.ivalue.append(f)

            if type_name == "texture2D":
                new_param.svalue = value
    
        if not db.set(id, rtr_mat.SerializeToString()):
            raise RuntimeError("Error: Could not set new rtr_material.")
        

if not db.close():
    raise RuntimeError("close error: " + str(db.error()))
