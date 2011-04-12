These script are experiments to generate a tree (as in nature) into a given
scene.

A tree can be expressed as a parametric L-system. The evolution of such 
a system generates a sequence of strings which can be interprete using a 
so-called 'turtle interpretation', which is a way of building a tree in three
dimensional space. Since we have a well-defined description of our *.rtr
format which is also accessible with python bindins, our turtle interpretation
simply writes into a scene.

This is how it works: 

    The turtle draws a 'line' when moving forward. This 'line' is a geometric 
    object that can be specified with its own rtr file (e.g. 
    ../../player/asset/segment_shape.rtr). This is basically the 
    building block each tree is made of. 

    Then you can build a scene and define placeholder nodes in it, which 
    are named for example "ternarytree_1". There are four types of tress 
    available.

    The script create_trees_for_scene will now take the two files and generate
    trees into the given prototype scene.

Here is short description of the scripts: 

    ./create_trees_for_scene.py
        Takes a scene contains the line geometric primitive and a scene which
        trees shall be generated into.

    ./ternarytree.py
        The PL-model of a ternary tree as given by Prusinkiewicz, P., "The 
        Algorithmic Beauty of Plants", pg. 60.

    ./turtleinterpeter.py
        A generic 3D turtle interpreter which generates geometry in our 
        *.rtr scene format.

System requirements: 

    In order to execute the tree generation, you will need the following: 

        Python 2.7
        Kyotocabinet Python 2.7 bindings: 
            For Windows, just follow the instructions in  
                ../../external/kyoto_cabinet/python_27_module/README.txt

        You will also need the numpy python package which can be downloaded
        from here:
            http://new.scipy.org/download.html

Example: 

    Assuming the all *.rtr in the following are available (which they are 
    if you have checked out the main source tree), you can execute the
    following command from the root folder of the source:

    python tools/pltreegen/create_trees_for_scene.py -i player/assets/input_scene.rtr -o player/assets/tree_scene.rtr -c -s player/assets/segment_shape.rtr -k node-segment_geom-segment_instance -t "ternarytree_1 ternarytree_2 ternarytree_3 ternarytree_4" -b

    Since the "-b" option is specified, this command will generate baked trees, 
    i.e. trees described by only one large mesh. Note that this might take a
    while for python to deal with.

    There is a ready-to-execute version of this available for download under
        http://hfink.eu/pixelnoir
            
