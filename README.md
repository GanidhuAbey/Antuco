# Antuco
A game engine/renderer made from vulkan

## Build Instructions (Windows with Visual Studio)

### Windows (Visual Studio 2019)

- clone the repo (make sure to get the submodules with `--recursive`)
- open with project folder with visual studio
- open up the `CMakeSettings.json` and create a configuration, this project should work for both the `msvc` and `clang-cl` compiler but since i'm currently developing exclusively with the `clang-cl` compiler I won't know how long the project will support `msvc`. It is important to note that due to one of the submodules i'm using (assimp) `clang-cl` will throw a bunch of warnings, which is pretty annoying but you can edit their files to deal with it.
- ideally, saving this file should now generate the ninja files and then you can build the project and run.

If you get an error like `assimp-vc142-mtd.lib could not be found` try checking if theres a file with that name in the build directory and if their isn't you can manually drag it in from the `external/libraries` folder.

### Other Platforms

Unfortunately I haven't done any testing on other platforms so I'm not sure how well they would work, you'll most likely have to compile the submodules yourself to get the proper libraries that are supported for your platform, other than that this project is built with CMake using the Clang compiler so it shouldn't be impossible to get it working in different platform.

This project is built with CMake and Clang so ideally it should work on most operating systems. That being said I only tested so far on windows (with visual studio)

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


## Road Map
