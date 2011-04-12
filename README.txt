=============================
= Pixel Noir Real Time Demo =
=============================

Pixel Noir is real-time demo that was created for the real-time rendering
course at the Vienna University of Technology.

For more information about the project, check out http://hfink.eu/pixelnoir.

The project is split into two sub-projects: 

    1.) A so-called "bakery" that reads COLLADA files and bakes them into our 
        own simplified scene format.

    2.) The "player" which loads files in the previously mentioned format. 
        This player is real-time rendering application that allow to play back
        pre-recorded camera rides and enables you to freely move around as well.

    Note that the player is not considered as a general purpose engine or
    viewer. There are some choices in design that were made to support our
    main project, the Pixel Noir scene (e.g. the dust particle effect).

In order to compile the sources you will need at least the following tools: 

    Scons 1.3.1 stable (there seem to be problems with 2.x releases)
    Python 2.7
    Cheetah Template Engine (http://www.cheetahtemplate.org/)

You can use the accompanied SConscript for compiling both under Windows with
Visual Studio 2010 installed and under Linux. For VS2010 all required libs and
SDKs are included within the source tree. On linux you will have to install
them by yourself.

There is also a separate VS2010-native project under ide/VS2010/solution.sln.
Be aware though, that you still need to generate some sources first by 
executing:

$ scons build/src_generated

Also, the VS-native projects have to be updated manually when source files are
added or removed.

There are some more funky python script under tools (e.g. one for generating 
trees into a scene). There is a separate README in this folder.

Here is a short description of the root folder contents: 

    ./player
        Contains all sources and resource files of the player-engine.

    ./collada_bakery
        Contains a OpenCOLLADA-based tool that parses COLLADA and outputs a 
        binary custom format (*.rtr, which can be viewed with the player).

    ./tools
        Some python scripts required for code generation, release engineering
        and a PL-system based tree generator.

    ./common
        Files that are required by the player and bakery. This folder also 
        contains the description of our own file rtr-format.

    ./build
        Build output of all projects.

    ./SConstruct
        The main Scons file. Can be used to compile under Windows and Linux

    ./ide
        Contains VS2010-native projects.
        
    ./external
        External dependencies (libraries) for the player and the bakery. For 
        Windows, all required libs are included (preferably as static libs).
        Under Linux you will have to install these libs on your distro.
        
 If you have any further questions, feel free to drop us an email: 
 
    Heinrich Fink <hf (at) hfink (dot) eu>, 
    Thomas Weber <weber (dot) t (at) gmx (dot) at>  
