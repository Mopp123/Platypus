# Platypus

*Clone the repo using `--recurse-submodules`

Goal eventually: <br/>
Game engine system things + editor used more like "power user way"..
* Less mouse usage (which is still supported tho) -> hotkeys, vim motions, etc. <br/>
* Minimalistic -> enable/use only stuff you need.
* Extendable/expandable, easily configurable.

**Dependencies**
* stb_image.h
* stb_image_write.h
* json.hpp
* tiny_gltf.h
* utfcpp https://github.com/nemtrif/utfcpp

Desktop:
* glfw https://github.com/glfw/glfw/tree/master <br/>
* Vulkan Memory Allocator https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/master <br/>
* Freetype https://github.com/freetype/freetype
* Harfbuzz https://github.com/harfbuzz/harfbuzz/tree/main (Actually not used atm...)

Web:
* Emscripten


## How to build ##
**TODO: redo the whole build instructions'n stuff**


### Install Vulkan dev tools

**Ubuntu (probably same with Debian)**
```
sudo apt-get install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools
```

To build Freetype you also require Harfbuzz which both require installing: <br/>
```
sudo apt-get install automake libtool autoconf libfreetype6-dev libglib2.0-dev libcairo2-dev meson pkg-config gtk-doc-tools
```

After installing all above mentioned dependencies, you can build dependency libs with  `./build-dependencies.sh` <br/>
NOTE: Should probably switch to build deps along with the project or at least to use static libs instead of shared which are used atm! <br/>

Also using glslc for compiling shaders.
Didn't want to add it as submodule for now.
You can get the unofficial binaries from here: https://github.com/google/shaderc/blob/main/downloads.md
Copy the file bin/glslc into /usr/local/bin (Not sure do you need other files for now...)

**Arch**
```
sudo pacman -S vulkan-devel
```
