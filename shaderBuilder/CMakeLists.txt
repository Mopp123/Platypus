cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "shader-builder")
project(${PROJECT_NAME})

# Need to have absolute dirs so we cannot use '../' to get the engine directory here!
get_filename_component(PARENT_DIR ../ ABSOLUTE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(
    -Wall -g
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
