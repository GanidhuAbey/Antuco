# Antuco
A game engine/renderer made from vulkan

## Build Instructions 

Clone repo recursively to get all requried dependences.

#### Shaderc

Shaderc has several required third-party dependencies that do not get downloaded automatically. Inorder to get dependencies installed first, run the following in the root directory:

```
# use the correct pathname for python in your case.
py ./external/include/shaderc/utils/git-sync-deps
```

Afterwards, the program can be installed as a standard CMake project.

## Screenshots

These are various screenshots I took through different stages of development in the engine:

#### Implementation of 3D Perspective:

![First Month Of Development](antuco_screenshots/august_11_progress_shot.PNG?raw=true "3D Perspective Rendering")

#### Rendering Multiple Objects (With Depth Testing)

![Depth Testing](antuco_screenshots/august_12_progress_shot.PNG?raw=true "Multiple Objects")

#### Basic Diffuse Lighting

![Lighting](antuco_screenshots/lighting.PNG?raw=true "Diffuse Lighting")

#### Shadows

![Shadows](antuco_screenshots/shadow_mapping_1_light.png?raw=true "Shadows")

#### Model Rendering

![BMW](antuco_screenshots/bmw_current.png?raw=true "BMW")
credit to tyrant_monkey for the model

#### Microfacet Material Reflections

![Teapot](antuco_screenshots/microfacet_specular.JPG?raw=true "Teapot")


## Road Map
