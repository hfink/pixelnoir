The bakery is a very efficient way of parsing COLLADA files and converting
the COLLADA model of your scene into a custom binary format that is 
tailor-made for our specific purposes.

Currently, the bakery supports the following COLLADA features: 

    LINEAR, STEP and BEZIER animation.

    Triangulated geometry, including proper handling of submeshes.

    Cameras

    Lights (currently, the 3dsmax extra data specification of falloff and
    attenuation is preferred over the COLLADA standard way, this should be 
    fixed in the future, or at least made compatible).

    profile_COMMON materials, partly with some 3dsmax specific extra features
    such as normal map bump channels. Note that we also handle UV coordinate
    arbitration.

    Arbitrary extra-data access.

The quality of the importer is also given by its many workarounds to handle 
partly correct files that 3DSMax is giving us. Please note that the primary 
goal was to create a strongly performing pipeline between 3DSMax and our
own format. There might still be some code in the importer that is specific on
some 3DSMax extra-data profiles. However, it should be relatively easy to
broaden support by being more strict about the actual COLLADA standard.