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

#ifndef MESH_GENERATION_H
#define MESH_GENERATION_H

#include "common.h"
#include "Mesh.h"
#include "LayerSource.h"

/**
 * Create a single RGB triangle
 * @return MeshInitializer object.
 */
MeshInitializer create_triangle(LayerSourceInitializerMap& sourcesOut);


MeshInitializer create_screen_filling_plane(LayerSourceInitializerMap& sourcesOut);

/**
 * Create a planar grid.
 * @param w Number of vertices along the horizontal axis.
 * @param h Number of vertices along the vertical axis.
 * @return MeshInitializer object.
 */
MeshInitializer create_grid(int w, 
                            int h, 
                            LayerSourceInitializerMap& sourcesOut);

/**
 * Create a sphere mesh.
 * @param slices Number of slices.
 * @param stacks Number of stacks.
 * @return MeshInitializer object.
 */
MeshInitializer create_sphere(float r, 
                              int slices, 
                              int stacks, 
                              const string& id,
                              LayerSourceInitializerMap& sourcesOut);

/**
 * Create a torus mesh.
 * @param r1 Outer radius.
 * @param r2 Inner radius.
 * @param s1 Number of outer slices.
 * @param s2 Number of inner slices.
 * @return MeshInitializer object.
 */
MeshInitializer create_donut(float r1, 
                             float r2, 
                             int s1, 
                             int s2, 
                             const string& id,
                             LayerSourceInitializerMap& sourcesOut);

MeshInitializer create_wired_cube(const string& id,
                                  LayerSourceInitializerMap& sourcesOut);

#endif
