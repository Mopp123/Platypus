target_sources(
    ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Buffers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderPassInstance.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Shader.cpp
)
add_subdirectory(renderers)
add_subdirectory(platform)
