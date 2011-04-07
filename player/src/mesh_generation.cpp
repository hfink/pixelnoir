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

#include "mesh_generation.h"
#include <glm/gtc/type_ptr.hpp>

template <typename T>
LayerSourceInitializer create_layer_source(shared_array<T> data_array, const string& name, int vtx_count) {
    ArrayAdapter source_data(data_array, vtx_count*gltype_info<T>::components);
    LayerSourceInitializer source_initializer(name, source_data);
    return source_initializer;
}

MeshInitializer create_triangle(LayerSourceInitializerMap& sourcesOut)
{

    //we store all data tightly in one array, 
    //first positions, then colors

    // Vertex Data
    shared_array<vec4> data(new vec4[6]);
    data[4] = vec4(1.0,0.0,0.0,1);
    data[5] = vec4(0.0,1.0,0.0,1),
    data[6] = vec4(0.0,0.0,1.0,1);

    // Calculate the triangle vertex positions using sin & cos
    for (int i = 0; i < 3; ++i) {
        float angle = 2 * static_cast<float>(M_PI) * i / 3.0f;
        data[i] = vec4(cos(angle),sin(angle),0,1);
    }

    // initialize the sources, we store both, color and positions in
    // one layer source (one VBO), first positions, then colors, tightly packed
    ArrayAdapter source_data(data, 24);

    LayerSourceInitializer source_init("triangle_source", source_data);
    sourcesOut[source_init.get_id()] = source_init;

    // MeshInitializer
    MeshInitializer triangle_data("triangle",GL_TRIANGLES, 3);

    // Add layers
    triangle_data.add_layer<vec4>("vertex", 0, source_init.get_id());
    triangle_data.add_layer<vec4>("color", 12, source_init.get_id());

    return triangle_data;    
}


MeshInitializer create_screen_filling_plane(LayerSourceInitializerMap& sourcesOut)
{
    // Vertex Data
    shared_array<vec4> vdata(new vec4[4]);
    vdata[0] = vec4(-1,-1,0,1);
    vdata[1] = vec4( 1,-1,0,1);
    vdata[2] = vec4(-1, 1,0,1);
    vdata[3] = vec4( 1, 1,0,1);

    //Note: array adapter does not take byte size, but numer of elements 
    //as size parameter! That makes sense because its access is strongly
    //typed.
    ArrayAdapter vsource_data(vdata, 4 * 4);

    LayerSourceInitializer vtx_lyr_src("screen_filling_quad_source", 
                                       vsource_data);

    sourcesOut[vtx_lyr_src.get_id()] = vtx_lyr_src;

    // MeshInitializer
    MeshInitializer initializer("screen_filling_quad",GL_TRIANGLE_STRIP, 4);

    // Add layers
    initializer.add_layer<vec4>("vertex", 0, vtx_lyr_src.get_id());

    return initializer;    
}

/**
 * Helper function for calculating the number of indices in a grid.
 */
GLint grid_index_count(int w, int h)
{
    return w*h + w*(h-2) + 2*(h-2);
}

// Helper macro
#define POS(x,y) ((y)*w+(x)+offset)

/**
 * Fill an array with indices for a grid.
 * @param w Number of vertices along the horizontal axis.
 * @param h Number of vertices along the vertical axis.
 */
void create_grid_indices(GLuint* dst, int w, int h, GLuint offset=0)
{
    int i = 0;

    for (int y = 0; y < h-1; ++y) {
        if (y > 0) {
            dst[i] = POS(w-1,y-1); ++i;
            dst[i] = POS(0,y+1); ++i;
        }

        for (int x = 0; x < w; ++x) {
            dst[i] = POS(x,y+1); ++i;
            dst[i] = POS(x,y); ++i;
        }
    }

    assert(i == grid_index_count(w,h));
}

MeshInitializer create_grid(int w, 
                            int h, 
                            LayerSourceInitializerMap& sourcesOut)
{
    int vtx_count = (w+1)*(h+1);
    shared_array<vec3> vp( new vec3[vtx_count] );
    shared_array<vec3> np( new vec3[vtx_count] );
    shared_array<vec3> tp( new vec3[vtx_count] );
    shared_array<vec3> cp( new vec3[vtx_count] );

    for (int i = 0; i <= h; ++i) {
        for (int j = 0; j <= w; ++j) {

            vp[i*(w+1)+j] = vec3(1-2*(float(j)/w), 
                                      0, 
                                      -1+2*(float(i)/h));
            np[i*(w+1)+j] = vec3(0,1,0);
            tp[i*(w+1)+j] = vec3(1,0,0);

            cp[i*(w+1)+j] = vec3(float(j)/w,
                                      float(i)/h,
                                      0);
        }
    }

    int index_count = grid_index_count(w+1,h+1);

    //Note that we have to use a shared array here, because the VBO
    //for the index buffer is created at a later point
    shared_array<GLuint> ip(new GLuint[index_count]);
    ArrayAdapter source_idx_data(ip, index_count);
    create_grid_indices(ip.get(), w+1, h+1);

    //NOTE: we could actually pack all data into one VBO,
    //however, it was easier to just take the original arrays and use 
    //multiple VBOs for now.
    LayerSourceInitializer vtx_layer_source = create_layer_source(vp, "grid_vertices_source", vtx_count);
    LayerSourceInitializer nml_layer_source = create_layer_source(np, "grid_normals_source", vtx_count);
    LayerSourceInitializer tng_layer_source = create_layer_source(tp, "grid_tangents_source", vtx_count);
    LayerSourceInitializer clr_layer_source = create_layer_source(cp, "grid_colors_source", vtx_count);

    sourcesOut[vtx_layer_source.get_id()] = vtx_layer_source;
    sourcesOut[nml_layer_source.get_id()] = nml_layer_source;
    sourcesOut[tng_layer_source.get_id()] = tng_layer_source;
    sourcesOut[clr_layer_source.get_id()] = clr_layer_source;

    MeshInitializer init("grid", GL_TRIANGLE_STRIP, (w+1)*(h+1));
    init.add_layer<vec3>("vertex", 0, vtx_layer_source.get_id());
    init.add_layer<vec3>("normal", 0, nml_layer_source.get_id());
    init.add_layer<vec3>("tangent", 0, tng_layer_source.get_id());
    init.add_layer<vec3>("color", 0, clr_layer_source.get_id());

    init.add_indices(source_idx_data);

    return init;
}

MeshInitializer create_sphere(float r, 
                              int slices, 
                              int stacks, 
                              const string& id,
                              LayerSourceInitializerMap& sourcesOut)
{
    int vtx_count = (slices+1)*(stacks+1);

    // Layer data arrays
    shared_array<vec3> vp (new vec3[vtx_count]);
    shared_array<vec3> np (new vec3[vtx_count]);
    shared_array<vec3> tp (new vec3[vtx_count]);
    shared_array<vec3> cp (new vec3[vtx_count]);

    // Fill layers
    for (int i = 0; i <= stacks; ++i) {
        double eta = (1.0-(double)i/stacks)*M_PI;
        for (int j = 0; j <= slices; ++j) {
            double phi = ((double)j/slices)*2*M_PI;

            vp[i*(slices+1)+j] = vec3(sin(eta)*cos(phi), 
                                      sin(eta)*sin(phi), 
                                      cos(eta)) * r;
            np[i*(slices+1)+j] = vec3(sin(eta)*cos(phi), 
                                      sin(eta)*sin(phi), 
                                      cos(eta));

            tp[i*(slices+1)+j] = cross(vec3(cos(phi),sin(phi),0),
                                       vec3(0,0,1));

            double l = 1.0;//eta/(M_PI);

            cp[i*(slices+1)+j] = normalize(rgbColor(vec3(phi/(2*M_PI)*360, 
                                                         (l<0.5)?1.0:(1.0-(l-0.5)*2), 
                                                         (l>0.5)?1.0:(l*2))));
        }
    }

    int index_count = grid_index_count(slices+1,stacks+1);

    // Generate index data
    shared_array<GLuint> ip(new GLuint[index_count]);
    create_grid_indices(ip.get(), slices+1, stacks+1);
    ArrayAdapter source_idx_data(ip, index_count);

    LayerSourceInitializer vtx_layer_source = create_layer_source(vp, id + "_vertices_source", vtx_count);
    LayerSourceInitializer nml_layer_source = create_layer_source(np, id + "_normals_source", vtx_count);
    LayerSourceInitializer tng_layer_source = create_layer_source(tp, id + "_tangents_source", vtx_count);
    LayerSourceInitializer clr_layer_source = create_layer_source(cp, id + "_colors_source", vtx_count);

    sourcesOut[vtx_layer_source.get_id()] = vtx_layer_source;
    sourcesOut[nml_layer_source.get_id()] = nml_layer_source;
    sourcesOut[tng_layer_source.get_id()] = tng_layer_source;
    sourcesOut[clr_layer_source.get_id()] = clr_layer_source;

    // Pack data into a MeshInitializer object
    MeshInitializer init(id, GL_TRIANGLE_STRIP, vtx_count);
    init.add_layer<vec3>("vertex", 0, vtx_layer_source.get_id());
    init.add_layer<vec3>("normal", 0, nml_layer_source.get_id());
    init.add_layer<vec3>("tangent", 0, tng_layer_source.get_id());
    init.add_layer<vec3>("color", 0, clr_layer_source.get_id());

    init.add_indices(source_idx_data);

    return init;
}

MeshInitializer create_donut(float r1, 
                             float r2, 
                             int s1, 
                             int s2, 
                             const string& id,
                             LayerSourceInitializerMap& sourcesOut)
{
    int vtx_count = (s1+1)*(s2+1);
    // Setup layer data arrays
    shared_array<vec3> vp (new vec3[vtx_count]);
    shared_array<vec3> np (new vec3[vtx_count]);
    shared_array<vec3> tp (new vec3[vtx_count]);
    shared_array<vec3> cp (new vec3[vtx_count]);
    shared_array<vec2> tc (new vec2[vtx_count]);

    // Stretching parameter to minimze texture distortion.
    float tex_aspect = (float)int(r1/r2+0.5);

    // Fill layers
    for (int i = 0; i <= s1; ++i) {
        double outer = (double)i/s1 * 2*M_PI;

        vec3 dir1 = vec3(sin(outer),cos(outer),0);
        vec3 dir2 = vec3(0,0,1);

        vec3 tangent = cross(dir1,dir2);

        vec3 pos = r1 * dir1;

        for (int j = 0; j <= s2; ++j) {
            double inner = ((double)j/s2)*2*M_PI;

            vec3 normal = (float)cos(inner)*dir1 + 
                          (float)sin(inner)*dir2;

            vp[i*(s2+1)+j] = r2 * normal + pos;
            np[i*(s2+1)+j] = normal;
            tp[i*(s2+1)+j] = tangent;
            tc[i*(s2+1)+j] = vec2((float)i/s1 * tex_aspect, (float)j/s2);


            cp[i*(s2+1)+j] = rgbColor(vec3(outer/(2*M_PI)*360, 
                                           0.99,
                                           0.99));
        }
    }

    // Setup index data
    int index_count = grid_index_count(s2+1,s1+1);

    shared_array<GLuint> ip(new GLuint[index_count]);
    create_grid_indices(ip.get(), s2+1, s1+1);
    ArrayAdapter source_idx_data(ip, index_count);

    LayerSourceInitializer vtx_layer_source = create_layer_source(vp, id + "_vertices_source", vtx_count);
    LayerSourceInitializer nml_layer_source = create_layer_source(np, id + "_normals_source", vtx_count);
    LayerSourceInitializer tng_layer_source = create_layer_source(tp, id + "_tangents_source", vtx_count);
    LayerSourceInitializer clr_layer_source = create_layer_source(cp, id + "_colors_source", vtx_count);
    LayerSourceInitializer txc_layer_source = create_layer_source(tc, id + "_texcoords_source", vtx_count);

    sourcesOut[vtx_layer_source.get_id()] = vtx_layer_source;
    sourcesOut[nml_layer_source.get_id()] = nml_layer_source;
    sourcesOut[tng_layer_source.get_id()] = tng_layer_source;
    sourcesOut[clr_layer_source.get_id()] = clr_layer_source;
    sourcesOut[txc_layer_source.get_id()] = txc_layer_source;

    // Pack everything into a MeshInitializer object.
    MeshInitializer init(id, GL_TRIANGLE_STRIP, vtx_count);
    init.add_layer<vec3>("vertex", 0, vtx_layer_source.get_id());
    init.add_layer<vec3>("normal", 0, nml_layer_source.get_id());
    init.add_layer<vec3>("tangent", 0, tng_layer_source.get_id());
    init.add_layer<vec3>("color", 0, clr_layer_source.get_id());
    init.add_layer<vec2>("tex_coord", 0, txc_layer_source.get_id());

    init.add_indices(source_idx_data);

    return init;
}

MeshInitializer create_wired_cube(const string& id,
                                  LayerSourceInitializerMap& sourcesOut) {

    shared_array<GLuint> ip(new GLuint[24]);
    shared_array<vec3> vp (new vec3[8]);

    vp[0] = vec3(1, -1, -1);
    vp[1] = vec3(1, 1, -1);
    vp[2] = vec3(-1, 1, -1);
    vp[3] = vec3(-1, -1, -1);
    vp[4] = vec3(1, -1, 1);
    vp[5] = vec3(1, 1, 1);
    vp[6] = vec3(-1, 1, 1);
    vp[7] = vec3(-1, -1, 1);
     
    //bottom quad
    ip[0] = 0;
    ip[1] = 1;
    ip[2] = 2;
    ip[3] = 3;
    ip[4] = GPUMesh::PRIMITIVE_RESTART_IDX();

    //top quad
    ip[5] = 4;
    ip[6] = 5;
    ip[7] = 6;
    ip[8] = 7;
    ip[9] = GPUMesh::PRIMITIVE_RESTART_IDX();

    //lines connecting top with bottom
    ip[10] = 0;
    ip[11] = 4;
    ip[12] = GPUMesh::PRIMITIVE_RESTART_IDX();

    ip[13] = 3;
    ip[14] = 7;
    ip[15] = GPUMesh::PRIMITIVE_RESTART_IDX();

    ip[16] = 2;
    ip[17] = 6;
    ip[18] = GPUMesh::PRIMITIVE_RESTART_IDX();

    ip[19] = 1;
    ip[20] = 5;
    //end of primitive

    ArrayAdapter source_idx_data(ip, 21);

    LayerSourceInitializer vtx_layer_source = 
        create_layer_source( vp, id + "_vertices_source", 8);

    sourcesOut[vtx_layer_source.get_id()] = vtx_layer_source;

    MeshInitializer init(id, GL_LINE_LOOP, 24);
    init.add_layer<vec3>("vertex", 0, vtx_layer_source.get_id());

    init.add_indices(source_idx_data);

    return init;
}
