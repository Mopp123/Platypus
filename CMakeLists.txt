cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "platypus")

project(${PROJECT_NAME})


if(${BUILD_TARGET} MATCHES "desktop")
    message("Selected desktop build target")
elseif(${BUILD_TARGET} MATCHES "web")
    message(FATAL_ERROR "Web build not supported yet!")
else()
    message(FATAL_ERROR "No build target specified!")
endif()


# -g to to use gdb debugger
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g")
# For our vim setup's YCM plugin to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(PLATYPUS_DEBUG=1)

include_directories(
    ${PROJECT_SOURCE_DIR}
    "dependencies/glfw/include"
)

file(GLOB SRC_FILES
    platypus/core/*.cpp
    platypus/utils/*.cpp
    platypus/core/platform/${BUILD_TARGET}/*.cpp
    platypus/graphics/platform/${BUILD_TARGET}/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

# TODO: Some flag to build looking all libs from current dir for "shipping"
add_library(glfw SHARED IMPORTED)
set_property(TARGET glfw PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/dependencies/glfw/build/src/libglfw.so.3.5")

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)
