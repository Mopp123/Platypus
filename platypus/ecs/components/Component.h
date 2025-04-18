#pragma once

#include <string>


namespace platypus
{
    enum ComponentType
    {
        COMPONENT_TYPE_EMPTY = 0x0,
        COMPONENT_TYPE_TRANSFORM = 0x1,
        COMPONENT_TYPE_GUI_TRANSFORM = 0x2,
        COMPONENT_TYPE_STATIC_MESH_RENDERABLE = 0x4,
        COMPONENT_TYPE_GUI_RENDERABLE = 0x8,
        COMPONENT_TYPE_CAMERA = 0x10,
        COMPONENT_TYPE_DIRECTIONAL_LIGHT = 0x20
    };

    std::string component_type_to_string(ComponentType type);
}
