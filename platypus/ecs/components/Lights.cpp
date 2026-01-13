#include "Lights.h"
#include "platypus/core/Application.hpp"
#include "platypus/core/Scene.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    std::string light_type_to_string(LightType type)
    {
        switch (type)
        {
            case LightType::DIRECTIONAL_LIGHT: return "DIRECTIONAL_LIGHT";
            case LightType::POINT_LIGHT: return "POINT_LIGHT";
            case LightType::SPOT_LIGHT: return "SPOT_LIGHT";
            default: return "<Invalid type>";
        }
    }

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        const Matrix4f& shadowProjectionMatrix,
        const Matrix4f& shadowViewMatrix,
        bool enableShadows,
        float maxShadowDistance
    )
    {
        Scene* pScene = Application::get_instance()->getSceneManager().accessCurrentScene();
        if (!pScene->isValidEntity(target, "create_directional_light"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_LIGHT;
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
        Light* pDirectionalLight = (Light*)pComponent;
        pDirectionalLight->shadowProjectionMatrix = shadowProjectionMatrix;
        pDirectionalLight->shadowViewMatrix = shadowViewMatrix;
        pDirectionalLight->direction = direction;
        pDirectionalLight->color = color;
        pDirectionalLight->maxShadowDistance = maxShadowDistance;
        pDirectionalLight->type = LightType::DIRECTIONAL_LIGHT;
        pDirectionalLight->enableShadows = enableShadows;
        return pDirectionalLight;
    }

    Light* create_directional_light(
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
            false,
            0.0f
        );
    }
}
