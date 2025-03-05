# Platypus

## To build for desktop
**Dependencies**
* glfw https://github.com/glfw/glfw/tree/master <br/>
* Vulkan Memory Allocator https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/master <br/>
* stb_image.h
* stb_image_write.h
* json.hpp
* tiny_gltf.h

Clone the repo using `--recurse-submodules`

### Linux
You also require some Vulkan development tools. These depends on your distribution.<br/>
On Ubuntu: <br/>
```
sudo apt install vulkan-tools
sudo apt install libvulkan-dev
sudo apt install vulkan-validationlayers-dev
sudo apt install spirv-tools
```
Also using glslc for compiling shaders.
Didn't want to add it as submodule for now.
You can get the unofficial binaries from here: https://github.com/google/shaderc/blob/main/downloads.md
Copy the file bin/glslc into /usr/local/bin (Not sure do you need other files for now...)

## To build for web
**Dependencies**
* Emscripten
