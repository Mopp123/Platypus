#pragma once

#include <string>


namespace platypus
{
    enum ComponentType
    {
        COMPONENT_TYPE_EMPTY            = 0x0,
        COMPONENT_TYPE_TRANSFORM        = 0x1 << 1,
        COMPONENT_TYPE_GUI_TRANSFORM    = 0x1 << 2,
        COMPONENT_TYPE_RENDERABLE3D     = 0x1 << 3,
        COMPONENT_TYPE_GUI_RENDERABLE   = 0x1 << 4,
        COMPONENT_TYPE_CAMERA           = 0x1 << 5,
        COMPONENT_TYPE_LIGHT            = 0x1 << 6,
        COMPONENT_TYPE_SKELETAL_ANIMATION = 0x1 << 7,
        COMPONENT_TYPE_PARENT           = 0x1 << 8,
        COMPONENT_TYPE_CHILDREN         = 0x1 << 9,
        COMPONENT_TYPE_JOINT            = 0x1 << 10
    };

    std::string component_type_to_string(ComponentType type);
}
