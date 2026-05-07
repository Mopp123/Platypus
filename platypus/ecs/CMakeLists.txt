target_sources(
    ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Entity.cpp
)

add_subdirectory(components)
add_subdirectory(systems)
