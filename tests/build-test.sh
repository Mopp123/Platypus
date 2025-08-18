#!/bin/bash

# NOTE: To run this you first need to have done:
#   <path to emsdk>/emsdk activate latest
#   source <path to emsdk>/emsdk_env.sh

if [[ -z "$1" ]]
then
    echo -e "\e[31mBuild type required!\e[0m"
else
    build_type=$1
    echo "Building $build_type target..."

    if [[ "$build_type" == "web" ]]
    then
        echo -e "\e[93mNOTE: IMPORTANT: The web build is extremely slow if PLATYPUS_DEBUG is defined, since it causes every gl call to check for errors!\e[0m"
        emcmake cmake -S . -B build/web -DBUILD_TARGET=$build_type
        cmake --build build/web
    elif [[ "$build_type" == "desktop" ]]
    then
        cmake -S . -B build/desktop -DBUILD_TARGET=$build_type
        cmake --build ./build/desktop
    else
        echo -e "\e[31mUnsupported build type: $build_type\e[0m"
        echo "Currently supported build types:"
        echo "  web"
        echo "  desktop"
    fi
fi

