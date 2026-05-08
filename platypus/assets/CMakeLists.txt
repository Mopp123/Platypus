target_sources(
    ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/Asset.cpp
    ${CMAKE_CURRENT_LIST_DIR}/AssetManager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Font.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Image.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Material.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Mesh.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Texture.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SkeletalAnimationData.cpp
)
add_subdirectory(platform)
