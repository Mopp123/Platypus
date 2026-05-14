#include "Component.hpp"
#include "Camera.hpp"
#include "Lights.hpp"
#include "Renderable.hpp"
#include "SkeletalAnimation.hpp"
#include "Transform.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


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

    size_t get_component_serialized_size(ComponentType type)
    {
        switch (type)
        {
            case ComponentType::COMPONENT_TYPE_TRANSFORM: return serialized_transform_size;
            case ComponentType::COMPONENT_TYPE_GUI_TRANSFORM: return serialized_gui_transform_size;
            case ComponentType::COMPONENT_TYPE_RENDERABLE3D: return serialized_renderable3D_size;

            case ComponentType::COMPONENT_TYPE_GUI_RENDERABLE: {
                Debug::log(
                    "GUIRenderable Serialization not yet handled! (the string issue)",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return 0;
            }

            case ComponentType::COMPONENT_TYPE_CAMERA: return serialized_camera_size;
            case ComponentType::COMPONENT_TYPE_LIGHT: return serialized_light_size;
            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION: return serialized_skeletal_animation_size;
            case ComponentType::COMPONENT_TYPE_JOINT: return serialized_skeleton_joint_size;
            case ComponentType::COMPONENT_TYPE_PARENT: return serialized_parent_size;
            case ComponentType::COMPONENT_TYPE_CHILDREN: return serialized_children_size;
            default: return 0;
        }
    }

    std::vector<char> serialize_component(
        ComponentType componentType,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(get_component_size(componentType) == dataSize);

        // TODO: UNFUCK BELOW!
        switch (componentType)
        {
            case ComponentType::COMPONENT_TYPE_TRANSFORM:
                return serialize(reinterpret_cast<const Transform*>(pData));
            case ComponentType::COMPONENT_TYPE_GUI_TRANSFORM:
                return serialize(reinterpret_cast<const GUITransform*>(pData));
            case ComponentType::COMPONENT_TYPE_RENDERABLE3D:
                return serialize(reinterpret_cast<const Renderable3D*>(pData));

            case ComponentType::COMPONENT_TYPE_GUI_RENDERABLE: {
                Debug::log(
                    "GUIRenderable Serialization not yet handled! (the string issue)",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return { };
            }

            case ComponentType::COMPONENT_TYPE_CAMERA:
                return serialize(reinterpret_cast<const Camera*>(pData));

            case ComponentType::COMPONENT_TYPE_LIGHT:
                return serialize(reinterpret_cast<const Light*>(pData));

            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION:
                return serialize(reinterpret_cast<const SkeletalAnimation*>(pData));

            case ComponentType::COMPONENT_TYPE_JOINT:
                return serialize(reinterpret_cast<const SkeletonJoint*>(pData));

            case ComponentType::COMPONENT_TYPE_PARENT:
                return serialize(reinterpret_cast<const Parent*>(pData));

            case ComponentType::COMPONENT_TYPE_CHILDREN:
                return serialize(reinterpret_cast<const Children*>(pData));

            default: return { };
        }
    }

    void deserialize_component(
        Scene* pScene,
        ComponentType componentType,
        void** ppComponent,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        Debug::log(
            "___TEST___deserializing component: " + component_type_to_string(componentType)
        );
        PLATYPUS_ASSERT(dataSize == get_component_serialized_size(componentType));

        // TODO: UNFUCK BELOW!
        switch (componentType)
        {
            case ComponentType::COMPONENT_TYPE_TRANSFORM:
                deserialize(
                    pScene,
                    reinterpret_cast<Transform**>(ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_GUI_TRANSFORM:
                deserialize(
                    pScene,
                    reinterpret_cast<GUITransform**>(ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_RENDERABLE3D:
                deserialize(
                    pScene,
                    reinterpret_cast<Renderable3D**>(ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_GUI_RENDERABLE: {
                Debug::log(
                    "GUIRenderable Serialization not yet handled! (the string issue)",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            case ComponentType::COMPONENT_TYPE_CAMERA:
                deserialize(
                    pScene,
                    reinterpret_cast<Camera**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_LIGHT:
                deserialize(
                    pScene,
                    reinterpret_cast<Light**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_SKELETAL_ANIMATION:
                deserialize(
                    pScene,
                    reinterpret_cast<SkeletalAnimation**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_JOINT:
                deserialize(
                    pScene,
                    reinterpret_cast<SkeletonJoint**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_PARENT:
                deserialize(
                    pScene,
                    reinterpret_cast<Parent**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            case ComponentType::COMPONENT_TYPE_CHILDREN:
                deserialize(
                    pScene,
                    reinterpret_cast<Children**>(*ppComponent),
                    entityID,
                    dataSize,
                    pData
                );
                break;

            default:
            {
                Debug::log(
                    "Invalid component type " + component_type_to_string(componentType) + " "
                    "for deserialization",
                    PLATYPUS_CURRENT_FUNC_NAME,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                break;
            }
        }
    }
}
