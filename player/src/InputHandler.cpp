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

#include "InputHandler.h"
#include "Runtime.h"
#include "RtrPlayerConfig.h"
#include "Timer.h"

InputHandler* input_handler_callback_object = NULL;

void GLFWCALL input_handler_key_callback(int key, int action)
{
    assert(input_handler_callback_object != NULL);

    input_handler_callback_object->key_callback(key, action);
}

InputHandler::InputHandler(Runtime& runtime, Timer& timer) :
    _runtime(runtime), _timer(timer), 
    _render_cam_freelook(runtime.render_camera()),
    _cull_cam_freelook(runtime.cull_camera())
{
    glfwGetMousePos(&_last_mouse_pos.x, &_last_mouse_pos.y);

    _control_camera = false;
    
    _active_freelook = &_render_cam_freelook;
    _inactive_freelook = &_cull_cam_freelook;

}

void InputHandler::set_callbacks()
{
    input_handler_callback_object = this;

    glfwSetKeyCallback(input_handler_key_callback);
}

void InputHandler::key_callback(int key, int action)
{
    if (action == GLFW_RELEASE) return;

    if (key == GLFW_KEY_SPACE) {
        if (_control_camera) {
            _control_camera = false;
            _timer.play();
            cout << "Camera control DISABLED." << endl;
        } else {
            _control_camera = true;
            _render_cam_freelook.reset(_runtime.render_camera());
            _cull_cam_freelook.reset(_runtime.cull_camera());
            _timer.pause();
            cout << "Camera control ENABLED." << endl;
        }
    } else if (key == GLFW_KEY_ENTER) {
        if (_timer.paused()) {
            _timer.play();
            cout << "Movie playback UNPAUSED." << endl;
        } else {
            _timer.pause();
            cout << "Movie playback PAUSED." << endl;
        }
    } else if (key == GLFW_KEY_F3) {
        cout << "Toggle wireframe." << endl;
        config.set_draw_wireframe(!config.draw_wireframe());
    } else if (key == GLFW_KEY_F4) {
        if (config.use_depth_of_field()) {
            cout << "Depth of field effect DISABLED" << endl;

            _runtime.set_depth_of_field(false);
            config.set_use_depth_of_field(false);
        } else {
            cout << "Depth of field effect ENABLED" << endl;
            
            _runtime.set_depth_of_field(true);
            config.set_use_depth_of_field(true);
        }
    } else if (key == GLFW_KEY_F5) {
        cout << "Reloading materials." << endl;
        _runtime.reload_materials();
    } else if (key == GLFW_KEY_F6) {

        _runtime.toggle_observer_camera();
        
        if (_runtime.render_camera()->get_id() == Runtime::kObserverCameraName())
        {
            cout << "Switching to observer mode." << endl;
        } else 
        {
            cout << "Switch back to render mode." << endl;
        }

        _render_cam_freelook.reset(_runtime.render_camera());
        _cull_cam_freelook.reset(_runtime.cull_camera());

    } else if (key == GLFW_KEY_F7) {
        //Toggles the freelook input between cull and render camera
        //Usually render camera equals cull camera, and in this case, 
        //this switch has no effect. If, however, the observer camera is
        //active we can move only the observer to watch the effectiveness
        //of culling
        if (_active_freelook == &_render_cam_freelook) {
            _active_freelook = &_cull_cam_freelook;
            _inactive_freelook = &_render_cam_freelook;
            cout << "Freelook controls the cull camera." << endl;
        } else {
            _active_freelook = &_render_cam_freelook;
            _inactive_freelook = &_cull_cam_freelook;
            cout << "Freelook controls the view camera." << endl;
        }
    } else if (key == GLFW_KEY_F8) {
        if (config.draw_bounding_geometry()) {
            cout << "Deactivation bounding box geometry." << endl;
            config.set_draw_bounding_geometry(false);
        } else {
            cout << "Activating bounding box geometry." << endl;
            config.set_draw_bounding_geometry(true);
        }
    } else if (key == GLFW_KEY_RIGHT) {
        if (glfwGetKey(GLFW_KEY_LSHIFT)) {
            _timer.fast_forward();
        } else if (glfwGetKey(GLFW_KEY_LCTRL)) {
            _timer.slow_forward();
        } else {
            _timer.forward();
        }
    } else if (key == GLFW_KEY_LEFT) {
        if (glfwGetKey(GLFW_KEY_LSHIFT)) {
            _timer.fast_reverse();
        } else if (glfwGetKey(GLFW_KEY_LCTRL)) {
            _timer.slow_reverse();
        } else {
            _timer.reverse();
        }
    }
}

void InputHandler::update()
{
    float time_diff = float(_timer.real_diff());

    ivec3 strafe(0,0,0);

    if (glfwGetKey(config.freelook_strafe_keys()[0])) 
        strafe.x += 1;
    
    if (glfwGetKey(config.freelook_strafe_keys()[1])) 
        strafe.x -= 1;
    
    if (glfwGetKey(config.freelook_strafe_keys()[2])) 
        strafe.y += 1;
    
    if (glfwGetKey(config.freelook_strafe_keys()[3])) 
        strafe.y -= 1;

    if (glfwGetKey(config.freelook_strafe_keys()[4])) 
        strafe.z += 1;
    
    if (glfwGetKey(config.freelook_strafe_keys()[5])) 
        strafe.z -= 1;

    ivec2 mouse_pos;
    glfwGetMousePos(&mouse_pos.x, &mouse_pos.y);

    if (_control_camera) {
        
        ivec2 mouse_diff = mouse_pos - _last_mouse_pos;

        if (!config.mouse_catch() && 
            !glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
            mouse_diff = ivec2(0);
        }

        _active_freelook->move(mouse_diff.x, mouse_diff.y, strafe, time_diff);

        if (_active_freelook->cam() != _inactive_freelook->cam())
            _inactive_freelook->move(0, 0, ivec3(0), time_diff);

        _runtime.render_camera()->set_use_target(false);
    } else {
        _runtime.render_camera()->set_use_target(true);
    }

    float focus = config.dof_default_focus();

    if (glfwGetKey(GLFW_KEY_UP)) {
        focus += time_diff * 100;
    }

    if (glfwGetKey(GLFW_KEY_DOWN)) {
        focus -= time_diff * 100;
    }

    focus = rtr::max(0.001f, focus);

    config.set_dof_default_focus(focus);

    _last_mouse_pos = mouse_pos;    

    if (config.dust_debug()) {
        vec3 dust_move(0);
        vec3 dust_scale(0);

        if (glfwGetKey(GLFW_KEY_LSHIFT)) {
            if (glfwGetKey('H')) {
                dust_scale.x -= 1;
            }

            if (glfwGetKey('K')) {
                dust_scale.x += 1;
            }

            if (glfwGetKey('J')) {
                dust_scale.y -= 1;
            }

            if (glfwGetKey('U')) {
                dust_scale.y += 1;
            }

            if (glfwGetKey('G')) {
                dust_scale.z -= 1;
            }

            if (glfwGetKey('T')) {
                dust_scale.z += 1;
            }
        } else {
            if (glfwGetKey('H')) {
                dust_move.x -= 1;
            }

            if (glfwGetKey('K')) {
                dust_move.x += 1;
            }

            if (glfwGetKey('J')) {
                dust_move.y -= 1;
            }

            if (glfwGetKey('U')) {
                dust_move.y += 1;
            }

            if (glfwGetKey('G')) {
                dust_move.z -= 1;
            }

            if (glfwGetKey('T')) {
                dust_move.z += 1;
            }
        }

        DustParticles& particles = _runtime.get_particle_system();
        
        float strafe_speed = config.freelook_strafe_speed();
        
        vec3 center = particles.center();
        vec3 size = particles.size();

        vec3 new_center = center + dust_move * strafe_speed * time_diff;
        vec3 new_size = size + dust_scale * strafe_speed * time_diff;

        if (length(dust_move) > 0 || length(dust_scale) > 0) {
            cout << new_center << " " << new_size << endl;
        }

        particles.set_center(new_center);
        particles.set_size(new_size);    
    }
}
