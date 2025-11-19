#!/bin/bash

build_dir=$(dirname "$0")
cd $build_dir
build_dir=$(pwd)
echo "Compiling shaders into $build_dir"

#glslc -fshader-stage=vert StaticVertexShader.glsl -o StaticVertexShader.spv
#glslc -fshader-stage=frag StaticFragmentShader_ds.glsl -o StaticFragmentShader_ds.spv
#
#glslc -fshader-stage=vert StaticVertexShader_tangent.glsl -o StaticVertexShader_tangent.spv
#glslc -fshader-stage=frag StaticFragmentShader_dsn.glsl -o StaticFragmentShader_dsn.spv
#
#glslc -fshader-stage=vert SkinnedVertexShader.glsl -o SkinnedVertexShader.spv
#glslc -fshader-stage=frag SkinnedFragmentShader_ds.glsl -o SkinnedFragmentShader_ds.spv
#
#glslc -fshader-stage=vert TerrainVertexShader.glsl -o TerrainVertexShader.spv
#glslc -fshader-stage=frag TerrainFragmentShader_ds.glsl -o TerrainFragmentShader_ds.spv
#
#glslc -fshader-stage=vert TerrainVertexShader_tangent.glsl -o TerrainVertexShader_tangent.spv
#glslc -fshader-stage=frag TerrainFragmentShader_dsn.glsl -o TerrainFragmentShader_dsn.spv
#
#glslc -fshader-stage=vert GUIVertexShader.glsl -o GUIVertexShader.spv
#glslc -fshader-stage=frag GUIFragmentShader.glsl -o GUIFragmentShader.spv
#
#glslc -fshader-stage=frag FontFragmentShader.glsl -o FontFragmentShader.spv
#
## Shadow casting shaders
#glslc -fshader-stage=vert shadows/StaticVertexShader.glsl -o shadows/StaticVertexShader.spv
#glslc -fshader-stage=frag shadows/StaticFragmentShader.glsl -o shadows/StaticFragmentShader.spv
#
#glslc -fshader-stage=vert shadows/SkinnedVertexShader.glsl -o shadows/SkinnedVertexShader.spv
#glslc -fshader-stage=frag shadows/SkinnedFragmentShader.glsl -o shadows/SkinnedFragmentShader.spv
#
## Shadow receiving shaders
#glslc -fshader-stage=vert receiveShadows/SkinnedVertexShader.glsl -o receiveShadows/SkinnedVertexShader.spv
#glslc -fshader-stage=frag receiveShadows/SkinnedFragmentShader_ds.glsl -o receiveShadows/SkinnedFragmentShader_ds.spv
#
#glslc -fshader-stage=vert receiveShadows/StaticVertexShader.glsl -o receiveShadows/StaticVertexShader.spv
#glslc -fshader-stage=frag receiveShadows/StaticFragmentShader_ds.glsl -o receiveShadows/StaticFragmentShader_ds.spv
#
#glslc -fshader-stage=vert receiveShadows/TerrainVertexShader_tangent.glsl -o receiveShadows/TerrainVertexShader_tangent.spv
#glslc -fshader-stage=frag receiveShadows/TerrainFragmentShader_dsn.glsl -o receiveShadows/TerrainFragmentShader_dsn.spv

extension_out=.spv
echo 'Found files:'
find $build_dir -type f -print0 | while read -d $'\0' full_file_path; do
    filename=$(basename -- "$full_file_path")
    extension="${filename##*.}"
    filename="${filename%.*}"
    if [ $extension == 'vert' ] || [ $extension == 'frag' ]
    then
        without_extension=${full_file_path%.*}
        out_file=$without_extension$extension_out
        echo "Writing file: $out_file"
    else
        echo "Invalid extension: $extension"
    fi
done
