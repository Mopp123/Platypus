#pragma once

#include "platypus/utils/ID.h"


namespace platypus
{
    struct StaticMeshRenderable
    {
        ID_t meshID = NULL_ID;
        ID_t textureID = NULL_ID;
    };
}
