cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "test")
project(${PROJECT_NAME})

# Need to have absolute dirs so we cannot use '../' to get the engine directory here!
get_filename_component(PARENT_DIR ../ ABSOLUTE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")
# For my vim setup's YCM plugin to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dirs included by all build types
set(PROJECT_INCLUDES ${PARENT_DIR})

if(${BUILD_TARGET} MATCHES "desktop")
    list(
        APPEND PROJECT_INCLUDES
        ${PARENT_DIR}/dependencies/glfw/include
    )
    set(ADDITIONAL_LINK_FLAGS "-g")
    add_library(platypus SHARED IMPORTED)
    set_property(TARGET platypus PROPERTY IMPORTED_LOCATION "${PARENT_DIR}/build/libplatypus.so")
elseif(${BUILD_TARGET} MATCHES "web")
else()
    message(FATAL_ERROR "No build target specified")
    return()
endif()

include_directories(
    ${PROJECT_INCLUDES}
)

file(GLOB SRC_FILES
    "src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SRC_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${ADDITIONAL_LINK_FLAGS})

if(${BUILD_TARGET} MATCHES "desktop")
    target_link_libraries(${PROJECT_NAME} PUBLIC platypus)
endif()
