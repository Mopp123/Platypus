#!/bin/bash

build_dir=$(dirname "$0")
cd $build_dir
build_dir=$(pwd)
echo "Compiling shaders into $build_dir"

glslc -fshader-stage=vert StaticVertexShader.glsl -o StaticVertexShader.spv
glslc -fshader-stage=frag StaticFragmentShader_ds.glsl -o StaticFragmentShader_ds.spv

glslc -fshader-stage=vert StaticVertexShader_tangent.glsl -o StaticVertexShader_tangent.spv
glslc -fshader-stage=frag StaticFragmentShader_dsn.glsl -o StaticFragmentShader_dsn.spv

glslc -fshader-stage=vert SkinnedVertexShader.glsl -o SkinnedVertexShader.spv
glslc -fshader-stage=frag SkinnedFragmentShader_ds.glsl -o SkinnedFragmentShader_ds.spv

glslc -fshader-stage=vert TerrainVertexShader.glsl -o TerrainVertexShader.spv
glslc -fshader-stage=frag TerrainFragmentShader_ds.glsl -o TerrainFragmentShader_ds.spv

glslc -fshader-stage=vert GUIVertexShader.glsl -o GUIVertexShader.spv
glslc -fshader-stage=frag GUIFragmentShader.glsl -o GUIFragmentShader.spv

glslc -fshader-stage=frag FontFragmentShader.glsl -o FontFragmentShader.spv
