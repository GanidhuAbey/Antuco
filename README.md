# Antuco

A game engine/renderer made from vulkan

## Build Instructions

To get all required dependencies for shaderc, run the following python script:

`py %PROJECT_DIR%/external/include/shaderc/utils/git-sync-deps`

Clone repo recursively to get all requried dependences.

#### Shaderc

Shaderc has several required third-party dependencies that do not get downloaded automatically. Inorder to get dependencies installed first, run the following in the root directory:

```
# use the correct pathname for python in your case.
py ./external/include/shaderc/utils/git-sync-deps
```

Afterwards, the program can be installed as a standard CMake project.

## Screenshots

Multi-PBR materials:

<img src="https://ganidhuabey.github.io/assets/images/Antuco/pbr_materials.png">


Damaged Helmet ([model](https://github.com/KhronosGroup/glTF-Sample-Models/tree/main))

<img src="https://ganidhuabey.github.io/assets/images/Antuco/damagedHelmet_v3.png">

Gun ([model](https://artisaverb.info/PBT.html) by Andrew Maximov)

<img src="https://ganidhuabey.github.io/assets/images/Antuco/Gun.png">

## Road Map

Finish implementation for normal mapping and emission texturing

Restructure Pass System for easy addition of new passes

Add support for compute passes

Utilize asset caching to improve load times
