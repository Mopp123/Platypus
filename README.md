# Platypus

## To build

### Linux
**Required dependencies**<br/>
On Ubuntu<br/>
Requires some Vulkan tools
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
