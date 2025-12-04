cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "shader-builder")
project(${PROJECT_NAME})

# Need to have absolute dirs so we cannot use '../' to get the engine directory here!
get_filename_component(PARENT_DIR ../ ABSOLUTE)

# V 20 to be able to use designated initializers
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(
    -Wall -g -Wpedantic
)

set(SRC_FILES
    src/*.cpp
)

# TODO: Static lib
add_library(platypus SHARED IMPORTED)
set_property(TARGET platypus PROPERTY IMPORTED_LOCATION "${PARENT_DIR}/build/libplatypus.so")

include_directories(
    ${PARENT_DIR}
    src
)

file(
    GLOB USE_SRC_FILES
    ${SRC_FILES}
)

add_executable(${PROJECT_NAME} ${USE_SRC_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC platypus)
