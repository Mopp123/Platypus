#include "Transform.h"
#include "platypus/core/Application.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    Transform* create_transform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_TRANSFORM;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_transform "
                "Failed to allocate Transform component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        Transform* pTransform = (Transform*)pComponent;
        pTransform->globalMatrix = create_transformation_matrix(
            position,
            rotation,
            scale
        );

        return pTransform;
    }

    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_gui_transform"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_GUI_TRANSFORM;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_gui_transform "
                "Failed to allocate GUITransform component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        GUITransform* pTransform = (GUITransform*)pComponent;
        pTransform->position = position;
        pTransform->scale = scale;

        return pTransform;
    }
}
