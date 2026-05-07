#include "Component.hpp"
#include "Camera.hpp"
#include "Lights.hpp"
#include "Renderable.hpp"
#include "SkeletalAnimation.hpp"
#include "Transform.hpp"


namespace platypus
{
    std::string component_type_to_string(ComponentType type)
    {
        switch(type)
        {
            case ComponentType::COMPONENT_TYPE_EMPTY: return "Empty";
            case ComponentType::COMPONENT_TYPE_TRANSFORM: return "Transform";
            case ComponentType::COMPONENT_TYPE_GUI_TRANSFORM: return "GUITransform";
            case ComponentType::COMPONENT_TYPE_RENDERABLE3D: return "Renderable3D";
            case ComponentType::COMPONENT_TYPE_GUI_RENDERABLE: return "GUIRenderable";
            case ComponentType::COMPONENT_TYPE_CAMERA: return "Camera";
            case ComponentType::COMPONENT_TYPE_LIGHT: return "Light";
            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION: return "SkeletalAnimation";
            case ComponentType::COMPONENT_TYPE_PARENT: return "Parent";
            case ComponentType::COMPONENT_TYPE_CHILDREN: return "Children";
            case ComponentType::COMPONENT_TYPE_JOINT: return "Joint";
            default: return "Invalid Type";
        }
    }

    size_t get_component_size(ComponentType type)
    {
        switch (type)
        {
            case ComponentType::COMPONENT_TYPE_TRANSFORM: return sizeof(Transform);
            case ComponentType::COMPONENT_TYPE_GUI_TRANSFORM: return sizeof(GUITransform);
            case ComponentType::COMPONENT_TYPE_RENDERABLE3D: return sizeof(Renderable3D);
            case ComponentType::COMPONENT_TYPE_GUI_RENDERABLE: return sizeof(GUIRenderable);
            case ComponentType::COMPONENT_TYPE_CAMERA: return sizeof(Camera);
            case ComponentType::COMPONENT_TYPE_LIGHT: return sizeof(Light);
            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION: return sizeof(SkeletalAnimation);
            case ComponentType::COMPONENT_TYPE_PARENT: return sizeof(Parent);
            case ComponentType::COMPONENT_TYPE_CHILDREN: return sizeof(Children);
            case ComponentType::COMPONENT_TYPE_JOINT: return sizeof(Joint);
            default: return 0;
        }
    }
}
