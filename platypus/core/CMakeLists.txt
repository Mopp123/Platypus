target_sources(
    ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Debug.cpp
    ${CMAKE_CURRENT_LIST_DIR}/InputManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Memory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Scene.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SceneManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Timing.cpp
)
add_subdirectory(platform)
