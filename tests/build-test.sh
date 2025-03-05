#!/bin/bash

# NOTE: To run this you first need to have done:
#   <path to emsdk>/emsdk activate latest
#   source <path to emsdk>/emsdk_env.sh

root_dir=$( pwd )

if [[ -z "$1" ]]
then
    echo -e "\e[31mBuild type required!\e[0m"
else
    src_dir=$root_dir
    build_type=$1

    echo "Using source dir: $src_dir"

    if [[ "$build_type" == "web" ]]
    then
        cd $root_dir/web/build
        echo "starting $build_type build to $( pwd )"
        emcmake cmake -S ${src_dir} -B . -DBUILD_TYPE=$build_type
        cmake --build .
    elif [[ "$build_type" == "linux" ]]
    then
        cmake -S . -B ./desktop/ -DBUILD_TYPE=$build_type
        cmake --build ./desktop/
    else
        echo -e "\e[31mUnsupported build type: $build_type\e[0m"
        echo "Currently supported build types:"
        echo "  web"
        echo "  linux"
    fi
fi

