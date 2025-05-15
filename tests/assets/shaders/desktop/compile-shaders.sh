#!/bin/bash

echo "Compiling shaders..."

glslc -fshader-stage=vert StaticVertexShader.glsl -o StaticVertexShader.spv
glslc -fshader-stage=frag StaticFragmentShader.glsl -o StaticFragmentShader.spv

glslc -fshader-stage=vert GUIVertexShader.glsl -o GUIVertexShader.spv
glslc -fshader-stage=frag GUIFragmentShader.glsl -o GUIFragmentShader.spv

glslc -fshader-stage=frag FontFragmentShader.glsl -o FontFragmentShader.spv
