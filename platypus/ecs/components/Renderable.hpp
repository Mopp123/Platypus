#pragma once

#include "platypus/utils/UUID.hpp"
#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/assets/Mesh.hpp"
#include "platypus/assets/Material.hpp"
#include <string>
#include <unordered_map>


namespace platypus
{
    constexpr size_t serialized_renderable3D_size =
        sizeof(ComponentType) +
        sizeof(UUID_t) * 2;

    struct Renderable3D
    {
        UUID_t meshID = NULL_UUID;
        UUID_t materialID = NULL_UUID;
    };

    bool mesh_and_material_compatible(
        const Mesh* pMesh,
        const Material* pMaterial
    );

    bool mesh_and_material_compatible_verbose(
        const Mesh* pMesh,
        const Material* pMaterial,
        std::string& outError
    );

    std::unordered_map<UUID_t, EntityError> query_renderable3D_compatibility_errors(
        Scene* pScene,
        const Material* pMaterial
    );

    // NOTE: When adding strings, make sure to take into accout
    // the move constructor thing when allocating from component pool!
    // -> if using std::string it's underlying mem is nullptr initially!
    struct GUIRenderable
    {
        UUID_t textureID = NULL_UUID;
        UUID_t fontID = NULL_UUID;
        Vector4f color = Vector4f(1, 1, 1, 1);
        Vector4f borderColor = Vector4f(1, 1, 1, 1);
        float borderThickness = 0.0f;
        Vector2f textureOffset = Vector2f(0, 0);
        uint32_t layer = 0;

        // TODO: When serializing, need to replace bool with uint8_t
        // and some way to handle the text string!
        bool isText = false;
        // NOTE: Warning when initially allocating from component pool!
        // -> needs to be explicitly resized to have any space!
        std::string text;
    };

    Renderable3D* create_renderable3D(
        entityID_t target,
        UUID_t meshAssetID,
        UUID_t materialAssetID,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false,
        bool allowNullAssets = false // ATM JUST FOR TESTING Renderable3D creation in Editor
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        UUID_t textureID,
        UUID_t fontID,
        Vector4f color,
        Vector4f borderColor,
        float borderThickness,
        Vector2f textureOffset,
        uint32_t layer,
        bool isText,
        std::string text,
        Scene* pScene = nullptr
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        const Vector4f color,
        Scene* pScene = nullptr
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        UUID_t textureID,
        Scene* pScene = nullptr
    );

    std::vector<char> serialize(const Renderable3D* pRenderable);
    void deserialize(
        Scene* pScene,
        Renderable3D** ppRenderable,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );
}
