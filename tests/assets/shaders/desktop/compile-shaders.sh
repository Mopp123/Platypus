#!/bin/bash

echo "Compiling shaders..."

glslc -fshader-stage=vert TestVertexShader.glsl -o TestVertexShader.spv
glslc -fshader-stage=frag TestFragmentShader.glsl -o TestFragmentShader.spv

glslc -fshader-stage=vert GUIVertexShader.glsl -o GUIVertexShader.spv
glslc -fshader-stage=frag GUIFragmentShader.glsl -o GUIFragmentShader.spv

glslc -fshader-stage=frag FontFragmentShader.glsl -o FontFragmentShader.spv
