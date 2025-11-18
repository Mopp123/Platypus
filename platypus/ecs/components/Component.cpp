#include "Component.h"


namespace platypus
{
    std::string component_type_to_string(ComponentType type)
    {
        switch(type)
        {
            case ComponentType::COMPONENT_TYPE_EMPTY: return "Empty";
            case ComponentType::COMPONENT_TYPE_TRANSFORM: return "Transform";
            case ComponentType::COMPONENT_TYPE_RENDERABLE3D: return "Renderable3D";
            case ComponentType::COMPONENT_TYPE_CAMERA: return "Camera";
            case ComponentType::COMPONENT_TYPE_LIGHT: return "Light";
            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION: return "SkeletalAnimation";
            case ComponentType::COMPONENT_TYPE_PARENT: return "Parent";
            case ComponentType::COMPONENT_TYPE_CHILDREN: return "Children";
            case ComponentType::COMPONENT_TYPE_JOINT: return "Joint";
            default: return "Invalid Type";
        }
    }
}
