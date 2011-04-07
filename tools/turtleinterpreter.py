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
import re
import random
import numpy as np
import copy

# We set the path to include the generated rtr_format python files
sys.path.append(sys.path[0] + os.sep+".."+os.sep+"Debug"+os.sep+"generated")

import rtr_format_pb2
from kyotocabinet import *

class TurtleInterpreter:
    """This class interprets a string of Turtle commands.

    You can use it together with one of the L system generators.
    Unknown Commands will trigger an error."""
    def __init__(self,
                 input_file,
                 output_file,
                 compress_output,
                 segment_file,
                 segment_id,
                 startup_scene_key,
                 dim_scale,
                 bake_segments):
        """Initializes a new interpreter.

        Params:
            input_file:
                The rtr-File to load, which contains the prototype scene
            output_file:
                The destination to write the generated scene to.
            compress_output:
                If enabled, output values in DB will be compressed.
            segment_file:
                The rtr-File which contains the prototype mesh to be
                used as a line segment. Note that we might performe some
                operations on it, where we assume that vertices z <= 0 are
                considered bottom, and vertices z >= 1 are considere top.
                The first encountered mesh element is used as a prototype
            segment_id:
                The ID of the mesh which will serve as the line segment
                prototype.
            startup_scene_key:
                The key to a string that defines the scene to write into.
                This is usually c onstant defined in our framework, however,
                python has no access to the C++ header that defines this
                constant, therefore we have to define it manually.
                (usually this is "startup_scene").
            dim_scale: A scale parameter which allows to scale all dimensional
                 parameters of turtle parameters to a certain amount.
        """
        self.__output_file = output_file
        self.__compress_output = compress_output
        self.dim_scale = dim_scale

        # Setup turtle command structure
        random.seed()
        self._max_deviation = 0
        self._turtle_commands = {}
        self._bake_segments = bake_segments
        self._num = 0

        # These are the commands that we currently support
        self._turtle_commands["F"] = self.forward_draw
        self._turtle_commands["f"] = self.forward
        self._turtle_commands["+"] = self.rotate_y
        self._turtle_commands["&"] = self.rotate_x
        self._turtle_commands["/"] = self.rotate_z
        self._turtle_commands["!"] = self.set_line_width
        self._turtle_commands["["] = self.start_branch
        self._turtle_commands["]"] = self.complete_branch
        
        # Open DB with segment definition
        
        if not os.path.exists(segment_file):
            raise RuntimeError("File "+
                               segment_file +
                               " does not exist.")

        db = DB()
        if not db.open(segment_file+"#type=kch", DB.OREADER):
            raise RuntimeError("Cannot open DB: " + str(db.error()))

        # Get the segment element, convert to Mesh, and save

        line_mesh_data = db.get("segmentShape")
        if not line_mesh_data:
            raise RuntimeError("Could not get segment:" + str(db.error()))

        # Unserialize
        line_mesh = rtr_format_pb2.Mesh()
        line_mesh.ParseFromString(line_mesh_data)

        # Extract the layer data of the segment prototype
        # We extract only vertices, normals and texture coordinates (#1)
        self._line_mesh_vtx_lyr = rtr_format_pb2.LayerSource()
        self._line_mesh_nml_lyr = rtr_format_pb2.LayerSource()
        self._line_mesh_tex_lyr = rtr_format_pb2.LayerSource()
        
        for layer in line_mesh.layer:
            if (layer.name == "vertex"):
                if (layer.num_components != 3):
                    raise RuntimeError("Error: Vtx Layer"+
                                       " has to have 3 components.")
                
                if (layer.source_index != 0):
                    raise RuntimeError("Error: expecting source index "+
                                       "to be zero.")
                
                lyr_data = db.get(layer.source)
                
                if not lyr_data:
                    raise RuntimeError("Could not get vertex layer data!")
                    
                self._line_mesh_vtx_lyr.ParseFromString(lyr_data)
                
            if (layer.name == "normal"):
                if (layer.num_components != 3):
                    raise RuntimeError("Error: nml has to have 3 components.")
                if (layer.source_index != 0):
                    raise RuntimeError("Error: expecting source index to be "+
                                       "zero.")
                lyr_data = db.get(layer.source)
                if not lyr_data:
                    raise RuntimeError("Could not get normal layer data!")
                    
                self._line_mesh_nml_lyr.ParseFromString(lyr_data)
                
            if (layer.name == "tex_coord"):
                if (layer.num_components != 2):
                    raise RuntimeError("Error: tex has to have 2 components.")
                if (layer.source_index != 0):
                    raise RuntimeError("Error: expecting source index to be "
                                       +"zero.")
                lyr_data = db.get(layer.source)
                if not lyr_data:
                    raise RuntimeError("Could not texture layer data!")
                    
                self._line_mesh_tex_lyr.ParseFromString(lyr_data)
        

        if not (self._line_mesh_vtx_lyr and
                self._line_mesh_nml_lyr and
                self._line_mesh_tex_lyr):
            raise RuntimeError("Error: prototype layers of segment are "
                               +"not completely loaded!")

        # TODO: this restriction could be made less strict
        if len(line_mesh.layer) != 3:
            raise RuntimeError("Error: Line mesh must have only 3 vertex attrs.")

        self._mesh_variations = {}
        self._mesh_variations[(1, 1, 1)] = [line_mesh,
                                            self._line_mesh_vtx_lyr,
                                            self._line_mesh_nml_lyr,
                                            self._line_mesh_tex_lyr]


        # done loading the prototype segment element
        if not db.close():
            raise RuntimeError("Close error: " + str(db.error()))

        # Now open the scene where we should generate the trees into
        if not os.path.exists(input_file):
            raise RuntimeError("File "+ intput_file + " does not exist.")

        if not db.open(input_file+"#type=kch", DB.OREADER):
            raise RuntimeError("Open error: " + str(db.error()))

        # we will now copy the prototype over the destination
        print(output_file)
        if os.path.exists(output_file):
            os.remove(output_file)

        # copy db
        if not db.copy(output_file):
            raise RuntimeError("Error: could not copy the database to "
                               + options.output+".")

        # close the db
        if not db.close():
            raise RuntimeError("close error: " + str(db.error()))

        # Reopen previously copied DB, this time with write mode
        db_opts = "#opts=l"
        if compress_output:
            db_opts = db_opts + "z"
            
        if not db.open(output_file+"#type=kch"+db_opts,
                       DB.OWRITER | DB.OCREATE):
            raise RuntimeError("Open error: " + str(db.error()))

        # we need to enter the reference mesh and layers into the scene
        if not db.set(self._line_mesh_vtx_lyr.id,
                      self._line_mesh_vtx_lyr.SerializeToString()):
            raise RuntimeError("Get error: " + str(db.error()))

        if not db.set(self._line_mesh_nml_lyr.id,
                      self._line_mesh_nml_lyr.SerializeToString()):
            raise RuntimeError("Get error: " + str(db.error()))

        if not db.set(self._line_mesh_tex_lyr.id,
                      self._line_mesh_tex_lyr.SerializeToString()):
            raise RuntimeError("Get error: " + str(db.error()))        

        if not db.set(self._mesh_variations[(1, 1, 1)][0].id,
                      self._mesh_variations[(1, 1, 1)][0].SerializeToString()):
            raise RuntimeError("Get error: " + str(db.error())) 

        # load our destination scene

        startup_scene_name = db.get(startup_scene_key)
        if not startup_scene_name:
            raise RuntimeError("Get error: " + str(db.error()))

        startup_scene_data = db.get(startup_scene_name)
        if not startup_scene_data:
            raise RuntimeError("Get error: " + str(db.error()))

        self._scene = rtr_format_pb2.Scene()
        self._scene.ParseFromString(startup_scene_data)

        # save the current DB In class
        self._db = db
        
        # Done with initialization

    def _get_mesh_variation(self, length, width, previous_width):
        """ Generates mesh variations on demand.

        We do  not use transformations for that. Instead we adapt
        the mesh for each length/width pair. While this might depend
        on the L system that is interpreted, this is usually feasible."""
        
        key = (length, width, previous_width)
        if key not in self._mesh_variations: # generate new variation
            new_mesh = rtr_format_pb2.Mesh()
            # we also need to create three new layers
            # and knit them together
            unique_id = "line_mesh_var_"+str(len(self._scene.geometry))
            # copy from proto type
            new_mesh.CopyFrom(self._mesh_variations[(1,1, 1)][0])

            new_mesh.id = unique_id
            # remove old layer references
            del new_mesh.layer[:]

            # Create new vertex layer
            
            vtx_lyr = rtr_format_pb2.LayerSource()
            vtx_lyr.CopyFrom(self._line_mesh_vtx_lyr)
            vtx_lyr.id = unique_id + "_vtx_layer"
            del(vtx_lyr.float_data[:])

            vtx_att_lyr = new_mesh.layer.add()
            vtx_att_lyr.name = "vertex"
            vtx_att_lyr.source = vtx_lyr.id
            vtx_att_lyr.num_components = 3
            vtx_att_lyr.source_index = 0

            # Create new normal layer
            
            nml_lyr = rtr_format_pb2.LayerSource()
            nml_lyr.CopyFrom(self._line_mesh_nml_lyr)
            nml_lyr.id = unique_id + "_nml_layer"
            del(nml_lyr.float_data[:])

            nml_att_lyr = new_mesh.layer.add()
            nml_att_lyr.name = "normal"
            nml_att_lyr.source = nml_lyr.id
            nml_att_lyr.num_components = 3
            nml_att_lyr.source_index = 0

            # Create new tex layer
            # TODO: deal with tex layer properly
            tex_lyr = rtr_format_pb2.LayerSource()
            tex_lyr.CopyFrom(self._line_mesh_tex_lyr)
            tex_lyr.id = unique_id + "_tex_layer"
            del(tex_lyr.float_data[:])

            tex_att_lyr = new_mesh.layer.add()
            tex_att_lyr.name = "tex_coord"
            tex_att_lyr.source = tex_lyr.id
            tex_att_lyr.num_components = 2
            tex_att_lyr.source_index = 0            

            length_inv = 1/length
            
            vtx = self._line_mesh_vtx_lyr.float_data
            nml = self._line_mesh_nml_lyr.float_data
            tex = self._line_mesh_tex_lyr.float_data
            
            for x, y, z, xn, yn, zn, r, s in zip(vtx[::3],vtx[1::3],vtx[2::3],
                                                 nml[::3],nml[1::3],nml[2::3],
                                                 tex[::2],tex[1::2]):
                scale_factor = (1-z)*previous_width + z*width
                vtx_lyr.float_data.append(x*scale_factor)
                vtx_lyr.float_data.append(y*scale_factor)
                z_new = 0
                zn_new = 0
                if z <= 1:
                    z_new = z*length
                    zn_new= zn*length_inv
                else:
                    z_new = (z - 1)*width+length
                    zn_new = zn
                vtx_lyr.float_data.append(z_new)

                scale_factor_inv = 1/scale_factor
                nml_lyr.float_data.append(xn*scale_factor_inv)
                nml_lyr.float_data.append(yn*scale_factor_inv)
                nml_lyr.float_data.append(zn_new)

                tex_lyr.float_data.append(r)
                tex_lyr.float_data.append(s*length)

            # Write everything to db
            if not self._db.set(new_mesh.id, new_mesh.SerializeToString()):
                raise RuntimeError("Error: could not serialize new mesh.")

            if not self._db.set(vtx_lyr.id, vtx_lyr.SerializeToString()):
                raise RuntimeError("Error: could not serialize new vtx layer.")        

            if not self._db.set(nml_lyr.id, nml_lyr.SerializeToString()):
                raise RuntimeError("Error: could not serialize new nml layer.")
                
            if not self._db.set(tex_lyr.id, tex_lyr.SerializeToString()):
                raise RuntimeError("Error: could not serialize new tex layer.")            

            self._mesh_variations[key] = [new_mesh, vtx_lyr, nml_lyr, tex_lyr]
        
        return self._mesh_variations[key]

    def forward_draw(self, length):

        self._num = self._num + 1
        """Interpretation of F(a). Places a line segment and moves forward"""
        
        id_cnt = str(len(self._scene.geometry))

        lookup = self._get_mesh_variation(
                                length*self.dim_scale,
                                self._current_state.line_width*self.dim_scale,
                                self._current_state.previous_line_width*self.dim_scale)

        lookup_mesh = lookup[0]

        # we calculate the rotation only matrix earlier, as we
        # need it twice
        # as we know that the turtle matrix consists of rotations and
        # translations only, we can take the upper 3x3 as rotation only
        # matrix
        rot_only = self._current_state.turtle_matrix[0:3,0:3]
        
        if self._bake_segments:
            # Bake everything in our bake buffers

            vtx = lookup[1].float_data
            nml = lookup[2].float_data
            tex = lookup[3].float_data

            # Get the current matrix and calculate the proper normal
            # matrix

            model_m = self._current_state.turtle_matrix;
            
            for x, y, z, xn, yn, zn, r, s in zip(vtx[::3],vtx[1::3],vtx[2::3],
                                                 nml[::3],nml[1::3],nml[2::3],
                                                 tex[::2],tex[1::2]):

                t_vtx = model_m * np.array([[x], [y], [z], [1]])
                self._bake_vtx_lyr.float_data.append(t_vtx[0,0])
                self._bake_vtx_lyr.float_data.append(t_vtx[1,0])
                self._bake_vtx_lyr.float_data.append(t_vtx[2,0])

                t_nml = rot_only * np.array([[xn], [yn], [zn]])
                self._bake_nml_lyr.float_data.append(t_nml[0,0])
                self._bake_nml_lyr.float_data.append(t_nml[1,0])
                self._bake_nml_lyr.float_data.append(t_nml[2,0])

                self._bake_tex_lyr.float_data.append(r)
                self._bake_tex_lyr.float_data.append(s)

            # update the index data
            indices = self._mesh_variations[(1, 1, 1)][0].index_data
            offset = len(self._bake_mesh.index_data) / len(indices)
            vtx_length = len(vtx)/3
            nml_length = len(nml)/3
            tex_length = len(tex)/2

            for idx_vtx, idx_nml, idx_tex in zip(indices[::3],
                                                 indices[1::3],
                                                 indices[2::3]):
                self._bake_mesh.index_data.append(idx_vtx + offset*vtx_length)
                self._bake_mesh.index_data.append(idx_nml + offset*nml_length)
                self._bake_mesh.index_data.append(idx_tex + offset*tex_length)    

            self._bake_mesh.vertex_count = self._bake_mesh.vertex_count + vtx_length
            
        else:

            # new matrix
            plc_node = self._scene.node.add()
            plc_node.id= "segment_placement_" + str(len(self._scene.node))
            plc_node.dependency = self._root_node.id
            trafo = plc_node.transform.add()
            trafo.type = trafo.MATRIX
            trafo.id = plc_node.id + "_trafo"
            trafo.matrix.m00 = self._current_state.turtle_matrix[0, 0]
            trafo.matrix.m01 = self._current_state.turtle_matrix[0, 1]
            trafo.matrix.m02 = self._current_state.turtle_matrix[0, 2]
            trafo.matrix.m03 = self._current_state.turtle_matrix[0, 3]

            trafo.matrix.m10 = self._current_state.turtle_matrix[1, 0]
            trafo.matrix.m11 = self._current_state.turtle_matrix[1, 1]
            trafo.matrix.m12 = self._current_state.turtle_matrix[1, 2]
            trafo.matrix.m13 = self._current_state.turtle_matrix[1, 3]

            trafo.matrix.m20 = self._current_state.turtle_matrix[2, 0]
            trafo.matrix.m21 = self._current_state.turtle_matrix[2, 1]
            trafo.matrix.m22 = self._current_state.turtle_matrix[2, 2]
            trafo.matrix.m23 = self._current_state.turtle_matrix[2, 3]

            trafo.matrix.m30 = self._current_state.turtle_matrix[3, 0]
            trafo.matrix.m31 = self._current_state.turtle_matrix[3, 1]
            trafo.matrix.m32 = self._current_state.turtle_matrix[3, 2]
            trafo.matrix.m33 = self._current_state.turtle_matrix[3, 3]

            segment = self._scene.geometry.add()
            segment.id = "line_instance_"+id_cnt
            segment.transform_node = plc_node.id
            segment.material_id = "BON1"
            segment.mesh_id = lookup_mesh.id            
       

            # Before actually moving forward (or doing any other re-orientation of
            # the turtle), we need to create a new node
            # or else previous set meshes will be modified, too.
##           fwd_node = self._scene.node.add()
##            fwd_node.id= "fwd_node_" + str(len(self._scene.node))
##            fwd_node.dependency = self._current_state.transform_node.id
##            self._current_state.transform_node = fwd_node

        
        # line was set, move forward
        # print("forward_draw:"+str(length))
        self.forward(length)

        # TODO: make tropism optional

        # apply tropism
        t = self._tropism_vector
        # we transform them into our local coordinate system
        # NOTE: this should not be necessary
        # inv_m = self._current_orientation_matrix
        
        inv_m = np.linalg.inv(rot_only)
        t_local = inv_m * t
        # t_local =  t

        t_local = t_local / np.linalg.norm(t_local)
        
        # heading_v_world = self._current_orientation_matrix * heading
        local_rotate_axis = np.cross([0, 0, 1],t_local.transpose()[0])

        #print(local_rotate_axis)
        if np.linalg.norm(local_rotate_axis) == 0:
            return
    
        adjustment_angle = self._tropism_sensibility * np.linalg.norm(local_rotate_axis)
        adjustment_angle_deg = np.degrees(adjustment_angle)
        
        # Apply rotation to global state
        c = self._current_state.turtle_matrix
        tropism_matrix = self.__rotate_matrix(local_rotate_axis, adjustment_angle_deg)
        self._current_state.turtle_matrix = c * tropism_matrix

        # apply locally to scene
##        """if not self._bake_segments:
##            node = self._current_state.transform_node
##            rt_trafo = node.transform.add()
##            rt_trafo.type = rtr_format_pb2.Transform.ROTATE
##            rt_trafo.id = node.id+"_rotate_trop_"+str(len(node.transform))
##
##            axis_n = local_rotate_axis / np.linalg.norm(local_rotate_axis)
##            rt_trafo.rotate.axis.x = axis_n[0][0]
##            rt_trafo.rotate.axis.y = axis_n[0][1]
##            rt_trafo.rotate.axis.z = axis_n[0][2]
##            rt_trafo.rotate.angle = adjustment_angle_deg"""

        # memorize this line width
        self._current_state.previous_line_width = self._current_state.line_width        

    def forward(self, length):
##        """f(l) ... move forward, i.e. add a translate operation to the
##        current transform."""
        
        # print("forward:"+str(length))

        # Apply locally
##        """
##        if not self._bake_segments:
##            node = self._current_state.transform_node
##            fwd_trafo = node.transform.add()
##            fwd_trafo.type = rtr_format_pb2.Transform.TRANSLATE
##            fwd_trafo.id = node.id+"_translate_"+str(len(node.transform))
##            fwd_trafo.translate.value.x = 0
##            fwd_trafo.translate.value.y = 0
##            fwd_trafo.translate.value.z = length*self.dim_scale"""

        # Apply to turtle matrix
        fwd_m = np.matrix([[1, 0, 0, 0],
                           [0, 1, 0, 0],
                           [0, 0, 1, length*self.dim_scale],
                           [0, 0, 0, 1]])

        c = self._current_state.turtle_matrix

        self._current_state.turtle_matrix = c*fwd_m

    def rotate_z(self, angle):
        """Interpretation of /(a). Rotate around local Z.

        Local Z equals the H axis of the turtle."""
        
        # print("rotate_z:"+str(angle))
##        """
##        if not self._bake_segments:
##            node = self._current_state.transform_node
##            rt_trafo = node.transform.add()
##            rt_trafo.type = rtr_format_pb2.Transform.ROTATE
##            rt_trafo.id = node.id+"_rotate_z_"+str(len(node.transform))
##            rt_trafo.rotate.axis.x = 0
##            rt_trafo.rotate.axis.y = 0
##            rt_trafo.rotate.axis.z = 1
##            rt_trafo.rotate.angle = angle"""

        # also modify the current matrix
        c = self._current_state.turtle_matrix
        rot_mat = self.__rotate_matrix([[0, 0, 1]], angle)
        self._current_state.turtle_matrix = c * rot_mat

    def rotate_y(self, angle):
        """Interpretation of +(a). Rotate around local Y.

        Local Y equals the U axis of the turtle."""
        # print("rotate_y:"+str(angle))
##        """
##        if not self._bake_segments:
##            node = self._current_state.transform_node
##            rt_trafo = node.transform.add()
##            rt_trafo.type = rtr_format_pb2.Transform.ROTATE
##            rt_trafo.id = node.id+"_rotate_y_"+str(len(node.transform))
##            rt_trafo.rotate.axis.x = 0
##            rt_trafo.rotate.axis.y = 1
##            rt_trafo.rotate.axis.z = 0
##            rt_trafo.rotate.angle = angle
##        """
        # also modify the current matrix
        c = self._current_state.turtle_matrix
        rot_mat = self.__rotate_matrix([[0, 1, 0]], angle)
        self._current_state.turtle_matrix = c * rot_mat       

    def rotate_x(self, angle):
        """Interpretation of &(a). Rotate around local X.

        Local X equals the L axis of the turtle."""
        # print("rotate_x:"+str(angle))
##        """
##        if not self._bake_segments:
##            node = self._current_state.transform_node
##            rt_trafo = node.transform.add()
##            rt_trafo.type = rtr_format_pb2.Transform.ROTATE
##            rt_trafo.id = node.id+"_rotate_x_"+str(len(node.transform))
##            rt_trafo.rotate.axis.x = 1
##            rt_trafo.rotate.axis.y = 0
##            rt_trafo.rotate.axis.z = 0
##            rt_trafo.rotate.angle = angle
##"""
        # also modify the current matrix
        c = self._current_state.turtle_matrix
        rot_mat = self.__rotate_matrix([[1, 0, 0]], angle)
        self._current_state.turtle_matrix = c * rot_mat      

    def set_line_width(self, diameter):
        """Interpretation of !(l). Sets the current line segment widht."""
        if self._current_state.previous_line_width == -1:
            self._current_state.previous_line_width = diameter
    
        self._current_state.line_width = diameter

    def start_branch(self):
        """Interpretation of [. Pushes the turtle state to a stack."""

        # Note: for local transform building, we need to create an
        # extra node per branch or else dependencies would not be
        # handled correctly
##        """
##        if not self._bake_segments:
##            branch_node = self._scene.node.add()
##            branch_node.id= "start_branch_node_" + str(len(self._scene.node))
##            branch_node.dependency = self._current_state.transform_node.id
##            """

        self._state_stack.append(copy.copy(self._current_state))

##        if not self._bake_segments:
##            self._current_state.transform_node = branch_node

    def complete_branch(self):
        """Interpretation of ]. Pops a state from the turtle stack and makes
        it current."""
        
        self._current_state = self._state_stack.pop()

        # we also have to add another node, since following
        # direction changes of the turtle must not influence previous transforms
##        """
##        if not self._bake_segments:
##            branch_node = self._scene.node.add()
##            branch_node.id= "complete_branch_node_" + str(len(self._scene.node))
##            branch_node.dependency = self._current_state.transform_node.id
##            self._current_state.transform_node = branch_node
##        """
    def interpret(self,
                  lstring,
                  spawn_points_prefix,
                  tropism_vector,
                  tropism_sensibility,
                  tropism_randomization):
        """For a number of spawn points we interpret the string."""
        spawn_pattern = "^"+spawn_points_prefix+".*$"
        spawn_points = [x for x in self._scene.node
                                if re.match(spawn_pattern,x.id)]
        if not spawn_points:
            print("Error: No spawn points were retrieved.")
        for spawn_point in spawn_points:            

            root_node = self._scene.node.add()
            root_node.id = "tree_root_node_"+str(len(self._scene.node))
            root_node.dependency = spawn_point.id

            if self._bake_segments:
                self.__setup_bake_mesh()
                self._bake_geometry.transform_node = root_node.id

            self._root_node = root_node

            # Reset states
            self._current_state = TurtleState()
            self._state_stack = []            
            self._current_state.transform_node = root_node
            #Note: we implicitly assume, that the root node has no orientation
            # set, i.e. our default H orientation is (0, 0, 1), we grow
            # "upwards"
            
            rnd1 = (random.random()*2 - 1)*tropism_randomization
            rnd2 = (random.random()*2 - 1)*tropism_randomization
            rnd3 = (random.random()*2 - 1)*tropism_randomization
            
            self._tropism_vector = np.array([[tropism_vector[0]+rnd1],
                                             [tropism_vector[1]+rnd2],
                                             [tropism_vector[2]+rnd3]]);
            
            self._tropism_sensibility = tropism_sensibility

            turtle_pattern = "(?P<cmd>[Ff\+&/\!\[\]])(\((?P<arg>\d+(\.\d*)?)\))?"

            # validate input
            complete_pattern = "^("+turtle_pattern+")*$"

            check = re.match(complete_pattern, lstring)
            if not check:
                print("String contains invalid commands. Not interpreting.")
                return
            
            # Finally, iterate over each turtle command and interpret
            for match in re.finditer(turtle_pattern,lstring):
                # execute the command
                cmd = match.groupdict()["cmd"]
                arg = match.groupdict()["arg"]
                if arg != None:
                    self._turtle_commands[cmd](float(arg))
                else:
                    self._turtle_commands[cmd]()

            if self._bake_segments:
                if not self._db.set(self._bake_mesh.id,
                                    self._bake_mesh.SerializeToString()):
                    raise RuntimeError("Error: could not serialize new mesh.")

                if not self._db.set(self._bake_vtx_lyr.id,
                                    self._bake_vtx_lyr.SerializeToString()):
                    raise RuntimeError("Error: could not serialize new vtx layer.")        

                if not self._db.set(self._bake_nml_lyr.id,
                                    self._bake_nml_lyr.SerializeToString()):
                    raise RuntimeError("Error: could not serialize new nml layer.")
                    
                if not self._db.set(self._bake_tex_lyr.id,
                                    self._bake_tex_lyr.SerializeToString()):
                    raise RuntimeError("Error: could not serialize new tex layer.")
            
        # Print out how many segments we generated
        print("Generated " + str(len(self._mesh_variations)) + " types of meshes.")

        print("Have " +str(self._num))
                    
    def write_and_close(self):
        
        if not self._db.set(self._scene.name,
                            self._scene.SerializeToString()):
            print("Error: could not serialize written scene.")
            
        if not self._db.close():
            print >>sys.stderr, "Close error: " + str(db.error())

    # Helper function to determine rotation matrix around
    # arbitrary axis
    def __rotate_matrix(self, axis, angle):
        a = axis[0]
        a = a / np.linalg.norm(a)
        angleRad = np.radians(angle);
        c = np.cos(angleRad);
        s = np.sin(angleRad);

        m00 = a[0]*a[0]*(1-c) + c;
        m01 = a[0]*a[1]*(1-c) - a[2]*s;
        m02 = a[0]*a[2]*(1-c) + a[1]*s;

        m10 = a[1]*a[0]*(1-c)+a[2]*s;
        m11 = a[1]*a[1]*(1-c) + c;
        m12 = a[1]*a[2]*(1-c) - a[0]*s;

        m20 = a[0]*a[2]*(1-c)-a[1]*s;
        m21 = a[1]*a[2]*(1-c)+a[0]*s;
        m22 = a[2]*a[2]*(1-c)+c;

        m = np.matrix( [[m00, m01, m02, 0],
                        [m10, m11, m12, 0],
                        [m20, m21, m22, 0],
                        [0,   0,   0,   1]] )
        return m

    def __setup_bake_mesh(self):
        
        new_mesh = rtr_format_pb2.Mesh()
        # we also need to create three new layers
        # and knit them together
        unique_id = "tree_bake_mesh_"+str(len(self._scene.geometry))

        new_mesh.id = unique_id
        new_mesh.primitive_type = new_mesh.TRIANGLES
        new_mesh.vertex_count = 0
        
        # Create new vertex layer
        
        vtx_lyr = rtr_format_pb2.LayerSource()
        vtx_lyr.id = unique_id + "_vtx_layer"
        vtx_lyr.type = vtx_lyr.FLOAT

        vtx_att_lyr = new_mesh.layer.add()
        vtx_att_lyr.name = "vertex"
        vtx_att_lyr.source = vtx_lyr.id
        vtx_att_lyr.num_components = 3
        vtx_att_lyr.source_index = 0

        # Create new normal layer
        
        nml_lyr = rtr_format_pb2.LayerSource()
        nml_lyr.id = unique_id + "_nml_layer"
        nml_lyr.type = nml_lyr.FLOAT

        nml_att_lyr = new_mesh.layer.add()
        nml_att_lyr.name = "normal"
        nml_att_lyr.source = nml_lyr.id
        nml_att_lyr.num_components = 3
        nml_att_lyr.source_index = 0

        # Create new tex layer
        # TODO: deal with tex layer properly
        tex_lyr = rtr_format_pb2.LayerSource()
        tex_lyr.id = unique_id + "_tex_layer"
        tex_lyr.type = tex_lyr.FLOAT

        tex_att_lyr = new_mesh.layer.add()
        tex_att_lyr.name = "tex_coord"
        tex_att_lyr.source = tex_lyr.id
        tex_att_lyr.num_components = 2
        tex_att_lyr.source_index = 0

        # Create the geometry node
        self._bake_geometry = self._scene.geometry.add()
        id_cnt = str(len(self._scene.geometry))
        self._bake_geometry.id = "bake_tree_"+id_cnt
        self._bake_geometry.material_id = "BON1"
        self._bake_geometry.mesh_id = new_mesh.id

        self._bake_mesh = new_mesh
        self._bake_vtx_lyr = vtx_lyr
        self._bake_nml_lyr = nml_lyr
        self._bake_tex_lyr = tex_lyr

# Helper class to wrap the turtle state
class TurtleState:
    def __init__(self):
        self.line_width = -1 # Width of the turtle line to be draw
        self.previous_line_width = -1
        self.transform_node = None # The node to enter next local transform
        # wraps orientation and position
        self.turtle_matrix = np.matrix([[1, 0, 0, 0],
                                        [0, 1, 0, 0],
                                        [0, 0, 1, 0],
                                        [0, 0, 0, 1]])
    
