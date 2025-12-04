#!/bin/bash

build_dir=$(dirname "$0")
cd $build_dir
build_dir=$(pwd)
echo "Compiling shaders into $build_dir"

extension_out=.spv
echo 'Found files:'
find $build_dir -type f -print0 | while read -d $'\0' full_file_path; do
    filename=$(basename -- "$full_file_path")
    full_without_extension="${full_file_path%.*}"
    extension="${filename##*.}"
    filename="${filename%.*}"
    vertex_stage_identifier="VertexShader"
    fragment_stage_identifier="FragmentShader"
    if [ $extension == 'glsl' ]
    then
        write_file="$full_without_extension.spv"
        echo "Writing file: $write_file"
        if [[ $filename =~ "$vertex_stage_identifier" ]]
        then
            glslc -fshader-stage=vert $full_file_path -o $write_file
        elif [[ $filename =~ "$fragment_stage_identifier" ]]
        then
            glslc -fshader-stage=frag $full_file_path -o $write_file
        fi
    fi
done
