#include "Component.h"


namespace platypus
{
    std::string component_type_to_string(ComponentType type)
    {
        switch(type)
        {
            case ComponentType::COMPONENT_TYPE_EMPTY: return "Empty";
            case ComponentType::COMPONENT_TYPE_TRANSFORM: return "Transform";
            case ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE: return "StaticMeshRenderable";
            case ComponentType::COMPONENT_TYPE_CAMERA: return "Camera";
            case ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT: return "DirectionalLight";
            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION: return "SkeletalAnimation";
            case ComponentType::COMPONENT_TYPE_PARENT: return "Parent";
            case ComponentType::COMPONENT_TYPE_CHILDREN: return "Children";
            default: return "Invalid Type";
        }
    }
}
