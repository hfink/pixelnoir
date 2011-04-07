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

#include "common.h"

#include "Mesh.h"
#include "RtrPlayerConfig.h"
#include "Shader.h"

#include "mesh_generation.h"

#include "Image.h"
#include "Texture.h"

#include "rtr_format.pb.h"

#include "Viewport.h"

#include "UniformBuffer.h"

#include "LooseOctree.h"

#include "FreeLookController.h"

#include <sstream>
#include <iomanip>

#include <glm/gtx/transform.hpp>

#include <math.h>

#include "MaterialManager.h"

#include "Runtime.h"
#include "DBLoader.h"

#include "InputHandler.h"

#include "SoundController.h"

#include <stdexcept>

#include <IL/il.h>
#include <IL/ilut.h>
#include <kcfile.h>
#include <kcthread.h>

#include <sstream>

void test_ogl3(void);
void main_loop_offline_mode();
void main_loop_online_mode();
bool test_config();

/**
 * Main function.
 * @param argc Number of command line arguments.
 * @param argv Array of command line arguments.
 * @return 0 in case of successful execution, !=0 otherwise.
 */
int main (int argc, char** argv)
{

#ifdef ENABLE_WIN_MEMORY_LEAK_DETECTION
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

    // Read config file
    string config_filename = "player_config.txt";

    bool lf_result = RtrPlayerConfig::load_file(config_filename, config);
    if (!lf_result)
        std::cerr << "Could not load config file " << config_filename << std::endl;

    argc = config.parse_args(argc, argv);

    if (config.save_options() == true) {
        config.set_save_options(false);
        
        // Save config
        RtrPlayerConfig::save_file(config_filename, config);
    }

    // Set up GLFW
    glfwInit();

    // Set flags so GLFW creates the desired OpenGL context
    // This is provided by the generated extGL header

    GLuint profile;
    if (EXTGL_CORE_PROFILE) {
      profile = GLFW_OPENGL_CORE_PROFILE;
    } else {
      profile = GLFW_OPENGL_COMPAT_PROFILE;
    }

    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, EXTGL_MAJOR_VERSION);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, EXTGL_MINOR_VERSION);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, profile);


#ifdef DEBUG_OPENGL
    // Demanding a debug context with an opengl version lower than
    // 4.1 might cause the program to crash.
#ifdef GL_VERSION_4_1
        glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
#endif

    GLint window_mode;
    if (config.window_mode() == RtrPlayerConfig::WINDOWED) {
        // Live-resizing would be a pain to handle with FBOs, so it's disabled.
        glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE , GL_TRUE);

        window_mode = GLFW_WINDOW;
    } else {
        window_mode = GLFW_FULLSCREEN;
    }

    // Create window and OpenGL context.
    if (glfwOpenWindow(	config.window_width(), config.window_height(),
                        0,0,0,0,
                        0, 0,
                        window_mode) != GL_TRUE) {
        cerr << "Failed to initialize OpenGL window." << endl;
        glfwTerminate();
        return 1;
    }

    if (config.mouse_catch() ) {
        glfwDisable(GLFW_MOUSE_CURSOR);
    } else {
        glfwEnable(GLFW_MOUSE_CURSOR);
    }

    // Load extensions and OGL functions with ExtGL
    if (!init_opengl()) {
        glfwTerminate();
        return 1;
    }

    int swap_interval = config.swap_interval();

    if ( (swap_interval != 1) && 
          config.offline_render_mode() )
    {
        std::cout << "Warning: Forcing swap_interval setting to 1, as "
                  << "offline_render_mode option is active." << std::endl;
        swap_interval = 1;
    }

    // Set window title and swap interval.
    glfwSetWindowTitle(config.window_title().c_str());
    glfwSwapInterval(swap_interval);

    // Enable sRGB gamma correction for framebuffer output
    // glEnable(GL_FRAMEBUFFER_SRGB);

    //we enable primitive restarting
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(GPUMesh::PRIMITIVE_RESTART_IDX());

    // Try to set up error callback for ARB_debug_output.
    set_GL_error_callbacks();

    // Do some OpenGL tests.
    test_ogl3();

    // Test for configuration problems.
    bool success = test_config();

    if (success) {
        // Start main loop
        if (config.offline_render_mode())
            main_loop_offline_mode();
        else
            main_loop_online_mode();
    }

    // Close window and OpenGL context.
    glfwTerminate();

    // We might call this earlier, whenever we know we can get rid of all
    // protobuf's data.
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

void main_loop_offline_mode()
{
    bool running = true;

    Viewport viewport(config.aspect_ratio());
    viewport.set_resize_callbacks();

    DBLoader db_loader(config.input());

    if (!db_loader.initialize()) {
        cerr << "Could not initialize database." << endl;
        return;
    }

    string scene_id = db_loader.startup_scene_id();
    shared_ptr<rtr_format::Scene> scene;
    db_loader.read(scene_id, scene);

    if (!scene) {
        cerr << "No startup scene could be loaded." << endl;
        return;
    }

    //Check if the directory exists, if not, create it
    kyotocabinet::File::Status s;
    std::string dirpath = config.offline_render_mode_output_dir();
    bool result = kc::File::status(dirpath, &s);

    if (!result) {

        std::cout << "Directory " << dirpath << " does not exist, creating it." 
                  << std::endl;
        if (!kc::File::make_directory(dirpath)) {
            std::cerr << "Error: Cannot create directory '" << dirpath << "'." 
                      << std::endl;
        }

    }

    // Configure the ILut images
    // Since the scene is already loaded, we expect the IL already to be 
    // initialized.
    ilutInit();
    
    ILuint image_target = 0;
    ilGenImages(1, &image_target);
    ilBindImage(image_target);

    Runtime runtime(*scene, &db_loader, viewport);

    Timer timer(0);

    // We have to update runtime once before creating the input handler.
    runtime.update(timer);

    glEnable(GL_CULL_FACE);

    double time = 0;

    double delta_time = 1.0/config.offline_render_mode_fps();

    size_t num_total_frames = 
        config.offline_render_mode_fps() * 
        config.offline_render_mode_duration(); 

    std::ostringstream oss;
    oss << num_total_frames;
    size_t max_num_digits = oss.str().length();

    for (size_t frame = 0; 
         (frame < num_total_frames) && running;
         time += delta_time, ++frame)
    {
        timer.update_diff(delta_time);

        runtime.update(timer);

        runtime.draw();

        // Swap buffers, get errors
        glfwSwapBuffers();

        if (!ilutGLScreen()) {
            std::cerr << "Could not take a screenshot." << std::endl;
        } else {
            std::cout << "Frame @ " << time << " : " << frame << "/" 
                      << num_total_frames << std::endl;
        }

        //and save
        std::ostringstream oss;
        oss << frame;
        
        //construct the file path
        std::string filepath_str =  dirpath + "/offline_render.";

        //zero padding of title
        for (size_t i = oss.str().length()-1; i<max_num_digits; ++i)
            filepath_str += "0";
        
        filepath_str += oss.str() + ".tiff";
        
        if (!ilSaveImage(filepath_str.c_str())) {
            std::cerr << "Could not save image to '" << filepath_str << "'." 
                      << std::endl;
        }

        get_errors();

        // Check if the window has been closed
        running = running && !glfwGetKey( GLFW_KEY_ESC );
        running = running && !glfwGetKey( 'Q' );
        running = running && glfwGetWindowParam( GLFW_OPENED );        
    }

    //Cleanup IL stuff
    ilDeleteImages(1, &image_target);
}

void main_loop_online_mode()
{
    bool running = true;
    float fps, mspf;

    Viewport viewport(config.aspect_ratio());
    viewport.set_resize_callbacks();

    DBLoader db_loader(config.input());

    if (!db_loader.initialize()) {
        cerr << "Could not initialize database." << endl;
        return;
    }

    string scene_id = db_loader.startup_scene_id();
    shared_ptr<rtr_format::Scene> scene;
    db_loader.read(scene_id, scene);

    if (!scene) {
        cerr << "No startup scene could be loaded." << endl;
        return;
    }

    string audio_file = config.audio_dir() + "/" + config.background_sound();
    SoundController sound_controller( audio_file );

    if (!sound_controller.is_valid()) {
        cerr << "Error: Could not load background sound '" << audio_file 
             << "'."
             << endl;
    }

    Runtime runtime(*scene, &db_loader, viewport);

    Timer timer(glfwGetTime());

    // We have to update runtime once before creating the input handler.
    runtime.update(timer);

    InputHandler input_handler(runtime, timer);

    input_handler.set_callbacks();

    glEnable(GL_CULL_FACE);

    if (config.play_music()) {
        sound_controller.play();
    }

    while (running) {
        timer.update(glfwGetTime());

        runtime.update(timer);
        input_handler.update();


        runtime.draw();

        // Swap buffers, get errors
        glfwSwapBuffers();
        get_errors();
        calc_fps(fps, mspf);

        // Check if the window has been closed
        running = running && !glfwGetKey( GLFW_KEY_ESC );
        running = running && !glfwGetKey( 'Q' );
        running = running && glfwGetWindowParam( GLFW_OPENED );        
    }

    sound_controller.stop();
}

// A helper macro for printing OpenGL limits.
#define PRINT_GL_LIMIT(limit_name)                                 \
{                                                                  \
    int l;                                                         \
    glGetIntegerv((limit_name), &l);                               \
    std::cout << #limit_name << " = " << l << std::endl;           \
}

// Test if we got a valid forward compatible context (FCC)
void test_ogl3(void)
{
    GLint profile;

    // retrieve our OpenGL-version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // check if we have a core-profile
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    if (profile == GL_CONTEXT_CORE_PROFILE_BIT) {
        std::cout << "Having a core-profile" << std::endl;
    } else {
        std::cout << "Having a compatibility-profile" << std::endl;
    }
    
    if (config.print_gl_limits()) {
#ifdef GL_VERSION_4_1
        PRINT_GL_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS);
        PRINT_GL_LIMIT(GL_MAX_SUBROUTINES);
        PRINT_GL_LIMIT(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS);
        PRINT_GL_LIMIT(GL_MAX_VERTEX_STREAMS);
#endif
        PRINT_GL_LIMIT(GL_MAX_TEXTURE_IMAGE_UNITS);
        PRINT_GL_LIMIT(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
        PRINT_GL_LIMIT(GL_MAX_VERTEX_ATTRIBS);
        PRINT_GL_LIMIT(GL_MAX_GEOMETRY_INPUT_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_VARYING_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_UNIFORM_BLOCK_SIZE);
        PRINT_GL_LIMIT(GL_MAX_VERTEX_UNIFORM_BLOCKS);
        PRINT_GL_LIMIT(GL_MAX_GEOMETRY_UNIFORM_BLOCKS);
        PRINT_GL_LIMIT(GL_MAX_FRAGMENT_UNIFORM_BLOCKS);
        PRINT_GL_LIMIT(GL_MAX_VERTEX_UNIFORM_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS);
        PRINT_GL_LIMIT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
        PRINT_GL_LIMIT(GL_MAX_UNIFORM_BUFFER_BINDINGS);
        PRINT_GL_LIMIT(GL_MAX_DEPTH_TEXTURE_SAMPLES);
        PRINT_GL_LIMIT(GL_MAX_COLOR_TEXTURE_SAMPLES);
        PRINT_GL_LIMIT(GL_MAX_INTEGER_SAMPLES);

        cout << endl;
    }

    if (EXTGL_ARB_debug_output) {
        cout << "ARB_debug_output extension is supported." << endl;
        
        // string message = "This is a test. ;)";
        // glDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB,
        //                         GL_DEBUG_TYPE_OTHER_ARB,
        //                         42,
        //                         GL_DEBUG_SEVERITY_MEDIUM_ARB,
        //                         message.size(),
        //                         message.c_str());
                                
    } else {
        cout << "ARB_debug_output extension is NOT supported." << endl;
    }

    if (EXTGL_EXT_texture_filter_anisotropic) {
        cout << "EXT_texture_filter_anisotropic is supported" << endl;
    } else {
        cout << "EXT_texture_filter_anisotropic is NOT supported" << endl;
    }
}


bool test_config()
{
    int max_depth_samples;
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_samples);
    int max_color_samples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_samples);

    if (config.fsaa_samples() > max_depth_samples || 
        config.fsaa_samples() > max_color_samples) {
        cerr << "Config problem(fsaa_samples = " << config.fsaa_samples()
             << "): Your graphics card only supports "
             << max_color_samples << "X Anti Aliasing." << endl;
        return false;
    }

    if(config.fsaa_samples() < 1) {
        cerr << "Config problem(fsaa_samples = " << config.fsaa_samples()
             << "): You need at least one sample per pixel. " << endl;
        return false;
    }

    return true;
}
