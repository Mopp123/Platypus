target_sources(
    ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/FileUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ID.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Maths.cpp
    ${CMAKE_CURRENT_LIST_DIR}/StringUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Algorithms.cpp
)
add_subdirectory(controllers)
add_subdirectory(modelLoading)
