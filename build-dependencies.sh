#!/bin/bash


# Get script dir as absolute path
root_dir=$( dirname "$0" )
cd $root_dir

dependencies_dir=$( pwd )/dependencies
cd $dependencies_dir
pwd $dependencies_dir

# build glfw
echo "Building GLFW..."
cd glfw/
cmake -S . -B build -D BUILD_SHARED_LIBS=ON
cmake --build ./build/

cd $dependencies_dir
