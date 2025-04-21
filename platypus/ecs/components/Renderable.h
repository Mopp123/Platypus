#pragma once

#include "platypus/utils/ID.h"
#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"
#include <string>


namespace platypus
{
    struct StaticMeshRenderable
    {
        ID_t meshID = NULL_ID;
        ID_t textureID = NULL_ID;
    };

    // NOTE: When adding strings, make sure to take into accout
    // the move constructor thing when allocating from component pool!
    // -> if using std::string it's underlying mem is nullptr initially!
    struct GUIRenderable
    {
        ID_t textureID = NULL_ID;
        ID_t fontID = NULL_ID;
        Vector4f color = Vector4f(1, 0, 1, 1);
        Vector2f textureOffset = Vector2f(0, 0);
        uint32_t layer = 0;
        // NOTE: Warning when initially allocating from component pool!
        // -> needs to be explicitly resized to have any space!
        std::wstring text;
    };

    StaticMeshRenderable* create_static_mesh_renderable(
        entityID_t target,
        ID_t meshAssetID,
        ID_t textureAssetID
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        const Vector4f color
    );
}
