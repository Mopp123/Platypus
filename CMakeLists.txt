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

if(${BUILD_TARGET} MATCHES "desktop")
    add_compile_definitions(PLATYPUS_BUILD_DESKTOP=1)
elseif(${BUILD_TARGET} MATCHES "web")
    add_compile_definitions(PLATYPUS_BUILD_WEB=1)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}
    "dependencies/glfw/include"
    "dependencies/stb"
    "dependencies/json"
    "dependencies/tinygltf"
)

file(GLOB SRC_FILES
    platypus/core/*.cpp
    platypus/core/platform/${BUILD_TARGET}/*.cpp

    platypus/ecs/components/*.cpp
    platypus/utils/*.cpp
    platypus/utils/controllers/*.cpp

    platypus/assets/*.cpp
    platypus/assets/platform/${BUILD_TARGET}/*.cpp

    platypus/graphics/*.cpp
    platypus/graphics/platform/${BUILD_TARGET}/*.cpp
    platypus/graphics/renderers/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

# TODO: Some flag to build looking all libs from current dir for "shipping"
add_library(glfw SHARED IMPORTED)
set_property(TARGET glfw PROPERTY IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/dependencies/glfw/build/src/libglfw.so.3.5")
# Below so much more nice, but the issue with licences and static linking?
# add_subdirectory(dependencies/glfw)

find_package(Vulkan REQUIRED)
set(VMA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/VulkanMemoryAllocator/include")


#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-O3")
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} ${VMA_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw Vulkan::Vulkan)
