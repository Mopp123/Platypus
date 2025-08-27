#!/bin/bash

build_dir=$(dirname "$0")
cd $build_dir
build_dir=$(pwd)
echo "Compiling shaders into $build_dir"

glslc -fshader-stage=vert StaticVertexShader.glsl -o StaticVertexShader.spv
glslc -fshader-stage=frag StaticFragmentShader.glsl -o StaticFragmentShader.spv

glslc -fshader-stage=vert StaticHDVertexShader.glsl -o StaticHDVertexShader.spv
glslc -fshader-stage=frag StaticHDFragmentShader.glsl -o StaticHDFragmentShader.spv

glslc -fshader-stage=vert SkinnedVertexShader.glsl -o SkinnedVertexShader.spv
glslc -fshader-stage=frag SkinnedFragmentShader.glsl -o SkinnedFragmentShader.spv

glslc -fshader-stage=vert GUIVertexShader.glsl -o GUIVertexShader.spv
glslc -fshader-stage=frag GUIFragmentShader.glsl -o GUIFragmentShader.spv

glslc -fshader-stage=frag FontFragmentShader.glsl -o FontFragmentShader.spv
