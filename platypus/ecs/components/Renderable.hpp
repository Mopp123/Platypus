#pragma once

#include "platypus/utils/ID.hpp"
#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include <string>


namespace platypus
{
    struct Renderable3D
    {
        ID_t meshID = NULL_ID;
        ID_t materialID = NULL_ID;
    };

    // NOTE: When adding strings, make sure to take into accout
    // the move constructor thing when allocating from component pool!
    // -> if using std::string it's underlying mem is nullptr initially!
    struct GUIRenderable
    {
        ID_t textureID = NULL_ID;
        ID_t fontID = NULL_ID;
        Vector4f color = Vector4f(1, 1, 1, 1);
        Vector4f borderColor = Vector4f(1, 1, 1, 1);
        float borderThickness = 0.0f;
        Vector2f textureOffset = Vector2f(0, 0);
        uint32_t layer = 0;

        bool isText = false;
        // NOTE: Warning when initially allocating from component pool!
        // -> needs to be explicitly resized to have any space!
        std::string text;
    };

    Renderable3D* create_renderable3D(
        entityID_t target,
        ID_t meshAssetID,
        ID_t materialAssetID
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        ID_t textureID,
        ID_t fontID,
        Vector4f color,
        Vector4f borderColor,
        float borderThickness,
        Vector2f textureOffset,
        uint32_t layer,
        bool isText,
        std::string text
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        const Vector4f color
    );

    GUIRenderable* create_gui_renderable(
        entityID_t target,
        ID_t textureID
    );
}
