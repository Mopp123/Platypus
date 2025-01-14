cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME "test")
project(${PROJECT_NAME})

# Need to have absolute dirs so we cannot use '../' to get the engine directory here!
get_filename_component(PARENT_DIR ../ ABSOLUTE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -g")
# For our vim setup's YCM plugin to work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
    "${PARENT_DIR}"
    "${PARENT_DIR}/dependencies/glfw/include"
)

file(GLOB SRC_FILES
    "src/*.cpp"
)

add_library(platypus SHARED IMPORTED)
set_property(TARGET platypus PROPERTY IMPORTED_LOCATION "${PARENT_DIR}/build/libplatypus.so")

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC platypus)
