The 'player' is the actual render-engine that plays back *.rtr files. 

There are plenty of configurable parameters, which can be set either in
./player_config.txt or as a command line parameter to the player executable.

Most options should have a comment describing its effect. If you have the
source files available, ./player/src/config.xml represents the original model
of the configuration parameters.

Once you have launched the player it will play back any animation, if present.

The following controls are available: 

    'SPACE' ... Stops the animation and allows the camera to be moved via
                WASD (fwd/bkwd/left/right) and RF (up/down) keys. Note that
                focus of the camera will be lost, and you might have to focus
                manually after uncoupling (see next control binding).
                During uncoupled mode, the mouse controls the viewing 
                direction.

    'UP/DOWN' ... Move focus plane further/closer to camera.

    'Enter' ... Pauses the animation, without uncoupling the camera.

    'LEFT' ...  Play backwards

    'RIGHT' ... Play forwards

    'CTRL+LEFT/RIGHT' ... Slow-speed fwd/bckwd playback

    'SHIFT+LEFT/RIGHT' ... High-speed fwd/backwd playback

    'F3'    ... Toggle wireframe mode.

    'F4'    ... Toggle depth-of-field effect.

    'F5'    ... Reload all materials (attention: experimental feature).

    'F6'    ... Switch between 'observer' mode and normal mode. The 'observer'
                mode is an auxiliary camera that is able to observe the actual
                render camera. That way you can debug view frustum culling and
                other camera based algorithms more easily.

    'F7'    ... Enables WASDFR controls for either the observer or render
                camera. Use this option if you want to look through the
                observer camera, but move the render camera.

    'F8'    ... Toggle display of geometry bounding geometry.

  