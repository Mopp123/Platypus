cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "platypus")

project(${PROJECT_NAME})

# -g to to use gdb debugger
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g")

include_directories(
    "platypus"
)

file(GLOB SRC_FILES
    platypus/core/*.cpp
)
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
