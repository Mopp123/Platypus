cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "userTest")
project(${PROJECT_NAME})

# Need to have absolute dirs so we cannot use '../' to get the engine directory here!
get_filename_component(PARENT_DIR ../ ABSOLUTE)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g")
add_compile_options(
    -Wall -std=c++17 -g
)
set(ADDITIONAL_LINK_FLAGS " ")

# For my vim setup's YCM plugin to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dirs included by all build types
set(PROJECT_INCLUDES "${PARENT_DIR}")

set(SRC_FILES
    src/*.cpp
)

if(${BUILD_TARGET} MATCHES "desktop")
    list(
        APPEND PROJECT_INCLUDES
        ${PARENT_DIR}/dependencies/glfw/include
    )
    add_library(platypus SHARED IMPORTED)
    set_property(TARGET platypus PROPERTY IMPORTED_LOCATION "${PARENT_DIR}/build/libplatypus.so")
elseif(${BUILD_TARGET} MATCHES "web")
    add_compile_definitions(PLATYPUS_BUILD_WEB=1 PLATYPUS_DEBUG=1)
    list(
        APPEND PROJECT_INCLUDES
        ${PARENT_DIR}/dependencies/stb
        ${PARENT_DIR}/dependencies/json
        ${PARENT_DIR}/dependencies/tinygltf
    )
    # NOTE: Below doesn't work if using add_compile_options() instead! ...for some reason...
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_FREETYPE=1 -fexceptions")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
    set(HTML_SHELL_LOCATION "${PROJECT_SOURCE_DIR}/shell.html")
    # NOTE: This gets converted to camel case for some reason...
    # NOTE: html_out becomes the PROJ_NAME for some reason... was too afraid to delete it from here since
    # Emscripten recommends using the -o out.html
    set(HTML_OUT "userTest.html")

    set(ADDITIONAL_LINK_FLAGS "-fexceptions -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -o ${HTML_OUT} --shell-file ${HTML_SHELL_LOCATION} -sFORCE_FILESYSTEM --use-preload-plugins -sFETCH -sALLOW_MEMORY_GROWTH=1 -l websocket.js")
    # NOTE: Engine source is included as a part of the web build
    # -> specify all engine files here
    list(APPEND SRC_FILES
        ${PARENT_DIR}/platypus/core/*.cpp
        ${PARENT_DIR}/platypus/core/platform/${BUILD_TARGET}/*.cpp

        ${PARENT_DIR}/platypus/ecs/components/*.cpp
        ${PARENT_DIR}/platypus/ecs/systems/*.cpp
        ${PARENT_DIR}/platypus/utils/*.cpp
        ${PARENT_DIR}/platypus/utils/modelLoading/*.cpp
        ${PARENT_DIR}/platypus/utils/controllers/*.cpp

        ${PARENT_DIR}/platypus/assets/*.cpp
        ${PARENT_DIR}/platypus/assets/platform/${BUILD_TARGET}/*.cpp

        ${PARENT_DIR}/platypus/graphics/*.cpp
        ${PARENT_DIR}/platypus/graphics/platform/${BUILD_TARGET}/*.cpp
        ${PARENT_DIR}/platypus/graphics/renderers/*.cpp

        ${PARENT_DIR}/platypus/ui/*.cpp
    )
else()
    message(FATAL_ERROR "No build target specified")
    return()
endif()

include_directories(
    ${PROJECT_INCLUDES}
)

file(
    GLOB USE_SRC_FILES
    ${SRC_FILES}
)

add_executable(${PROJECT_NAME} ${USE_SRC_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${ADDITIONAL_LINK_FLAGS})


if(${BUILD_TARGET} MATCHES "desktop")
    target_link_libraries(${PROJECT_NAME} PUBLIC platypus)
endif()
