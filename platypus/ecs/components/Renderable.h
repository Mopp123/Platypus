#pragma once

#include "platypus/utils/ID.h"
#include "platypus/utils/Maths.h"


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
        Vector4f color = Vector4f(1, 0, 1, 1);
        uint32_t layer = 0;
    };
}
