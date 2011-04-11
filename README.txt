=============================
= Pixel Noir Real Time Demo =
=============================


This folder basically contains two projects: 

    1.) A so-called "bakery" that reads COLLADA files and bakes them into our own
        simplified scene format.

    2.) The "player" which loads files in the previously mentioned format. 
        This player is real-time rendering application that allow to play back
        pre-recorded camera rides and enables you to freely move around as well.

In order to compile the sources you will need at least the following: 

    Scons 1.3.1 stable
    Python 2.7
    Cheetah Template Engine (http://www.cheetahtemplate.org/)

You can use the accompanied SConscript for compiling both under Windows with
Visual Studio 2010 installed and under Linux.

There is also a separate VS2010-native project under ide/VS2010/solution.sln.
Be aware though, that you still need to generate some sources first with: 

> scons build/src_generated

There are some more funky python script under tools (e.g. one for generating 
trees into a scene). Some of these might refer to an older version of the 
framework and will be fixed in the near future.

Here is a short description of the folder contents: 

    ./player
        Contains all sources and resource files of the player-engine.

    ./collada_bakery
        Contains a OpenCOLLADA-based tool that parses COLLADA and outputs a 
        binary custom format (*.rtr, which can be viewed with the player).

    ./tools
        Some python scripts required for code generation, release engineering
        and a PL-system based tree generator (might not be functional atm).

    ./common
        Files that are required by the player and bakery.

    ./build
        Build output of all projects.

    ./SConstruct
        The main Scons file. Can be used to compile under Windows and Linux

    ./ide
        Contains VS2010 projects.
        
    ./external
        External dependencies (libraries) for the player and the bakery. For 
        Windows, all required libs are included (preferably as static libs).
        Under Linux you will have to install these libs on your distro.
        
 If you have any questions, drop us an email: 
 
    Heinrich Fink <hf (at) hfink (dot) eu>, 
    Thomas Weber <weber (dot) t (at) gmx (dot) at>  
