# Antuco
A game engine/renderer made from vulkan

## Build Instructions (Windows with Visual Studio)

### Windows (Visual Studio 2019)

- clone the repo (make sure to get the submodules with `--recursive`)
- open with project folder with visual studio
- go to the `CMakeSettings.json` file and set up the CMake configurations, just add a configuration for the Clang compiler and save the file to build the CMake
- if you get a "permission denied" error try running visual studio in admin mode
- if the cmake built without errors you should be able to run it!

If you get an error like `assimp-vc142-mtd.lib could not be found` try checking if theirs a file with that name in the build directory and if their isn't you can manually drag it in from the `external/libraries` folder.

### Other Platforms

Unfortunately I haven't done any testing on other platforms so I'm not sure how well they would work, you'll most likely have to compile the submodules yourself to get the proper libraries that are supported for your platform, other than that this project is built with CMake using the Clang compiler so it shouldn't be impossible to get it working in different platform.

This project is built with CMake and Clang so ideally it should work on most operating systems. That being said I only tested so far on windows (with visual studio)

## Screenshots

## Road Map
