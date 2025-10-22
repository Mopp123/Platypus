#include "Lights.h"
#include "platypus/core/Application.h"
#include "platypus/core/Scene.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    DirectionalLight* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        const Matrix4f& shadowProjectionMatrix,
        const Matrix4f& viewMatrix,
        bool enableShadows
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_directional_light"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_DIRECTIONAL_LIGHT;
        void* pComponent = pScene->allocateComponent(target, componentType);
        if (!pComponent)
        {
            Debug::log(
                "@create_directional_light "
                "Failed to allocate DirectionalLight component for entity: " + std::to_string(target),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        pScene->addToComponentMask(target, componentType);
        DirectionalLight* pDirectionalLight = (DirectionalLight*)pComponent;
        pDirectionalLight->shadowProjectionMatrix = shadowProjectionMatrix;
        pDirectionalLight->viewMatrix = viewMatrix;
        pDirectionalLight->direction = direction;
        pDirectionalLight->color = color;
        pDirectionalLight->enableShadows = enableShadows;
        return pDirectionalLight;
    }

    DirectionalLight* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color
    )
    {
        return create_directional_light(
            target,
            direction,
            color,
            Matrix4f(1.0f),
            Matrix4f(1.0f),
            false
        );
    }
}
