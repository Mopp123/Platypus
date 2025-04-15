# Platypus

## To build for desktop
**Dependencies**
* glfw https://github.com/glfw/glfw/tree/master <br/>
* Vulkan Memory Allocator https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/master <br/>
* stb_image.h
* stb_image_write.h
* json.hpp
* tiny_gltf.h
* Freetype https://github.com/freetype/freetype
* Harfbuzz https://github.com/harfbuzz/harfbuzz/tree/main

Clone the repo using `--recurse-submodules`

### Linux
You require some Vulkan development tools. These depends on your distribution.<br/>
On Ubuntu: <br/>
```
sudo apt-get install vulkan-tools libvulkan-dev vulkan-validationlayers-dev spirv-tools
```

To build Freetype you also require Harfbuzz which both require installing: <br/>
```
sudo apt-get install automake libtool autoconf libfreetype6-dev libglib2.0-dev libcairo2-dev meson pkg-config gtk-doc-tools
```

After installing all above mentioned dependencies, you can build dependency libs with  `./build-dependencies.sh` <br/>

Also using glslc for compiling shaders.
Didn't want to add it as submodule for now.
You can get the unofficial binaries from here: https://github.com/google/shaderc/blob/main/downloads.md
Copy the file bin/glslc into /usr/local/bin (Not sure do you need other files for now...)

## To build for web
**Dependencies**
* Emscripten
