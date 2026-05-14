#include "Lights.hpp"
#include "platypus/core/Application.hpp"
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
        float maxShadowDistance,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        Scene* pUseScene = pScene;
        if (!pUseScene)
            pUseScene = Application::get_instance()->getSceneManager().accessCurrentScene();

        if (!pUseScene->isValidEntity(target, "create_directional_light"))
        {
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        ComponentType componentType = ComponentType::COMPONENT_TYPE_LIGHT;
        void* pComponent = pUseScene->allocateComponent(target, componentType);
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
        if (!useExplicitComponentMask)
            pUseScene->addToComponentMask(target, componentType);

        Light* pDirectionalLight = (Light*)pComponent;
        pDirectionalLight->shadowProjectionMatrix = shadowProjectionMatrix;
        pDirectionalLight->shadowViewMatrix = shadowViewMatrix;
        pDirectionalLight->direction = direction;
        pDirectionalLight->color = color;
        pDirectionalLight->maxShadowDistance = maxShadowDistance;
        pDirectionalLight->type = LightType::DIRECTIONAL_LIGHT;
        pDirectionalLight->enableShadows = static_cast<uint8_t>(enableShadows);
        return pDirectionalLight;
    }

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        Scene* pScene,
        bool useExplicitComponentMask
    )
    {
        return create_directional_light(
            target,
            direction,
            color,
            Matrix4f(1.0f),
            Matrix4f(1.0f),
            false,
            0.0f,
            pScene,
            useExplicitComponentMask
        );
    }


    std::vector<char> serialize(const Light* pLight)
    {
        std::vector<char> serializedData(serialized_light_size);
        const ComponentType componentType = ComponentType::COMPONENT_TYPE_LIGHT;
        memcpy(
            serializedData.data(),
            &componentType,
            sizeof(ComponentType)
        );
        size_t pos = sizeof(ComponentType);

        memcpy(
            serializedData.data() + pos,
            &(pLight->shadowProjectionMatrix),
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            serializedData.data() + pos,
            &(pLight->shadowViewMatrix),
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            serializedData.data() + pos,
            &(pLight->direction),
            sizeof(Vector3f)
        );
        pos += sizeof(Vector3f);

        memcpy(
            serializedData.data() + pos,
            &(pLight->color),
            sizeof(Vector3f)
        );
        pos += sizeof(Vector3f);

        memcpy(
            serializedData.data() + pos,
            &(pLight->maxShadowDistance),
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            serializedData.data() + pos,
            &(pLight->type),
            sizeof(LightType)
        );
        pos += sizeof(LightType);

        memcpy(
            serializedData.data() + pos,
            &(pLight->enableShadows),
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);

        return serializedData;
    }


    void deserialize(
        Scene* pScene,
        Light** ppLight,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    )
    {
        PLATYPUS_ASSERT(pScene->entityExists(entityID));
        PLATYPUS_ASSERT(dataSize == serialized_light_size);

        ComponentType componentType;
        memcpy(&componentType, pData, sizeof(ComponentType));
        PLATYPUS_ASSERT(componentType == ComponentType::COMPONENT_TYPE_LIGHT);
        size_t pos = sizeof(ComponentType);

        Matrix4f shadowProjectionMatrix;
        Matrix4f shadowViewMatrix;
        Vector3f direction;
        Vector3f color;
        float maxShadowDistance;
        LightType type;
        uint8_t enableShadows;

        memcpy(
            &shadowProjectionMatrix,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            &shadowViewMatrix,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(Matrix4f)
        );
        pos += sizeof(Matrix4f);

        memcpy(
            &direction,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(Vector3f)
        );
        pos += sizeof(Vector3f);

        memcpy(
            &color,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(Vector3f)
        );
        pos += sizeof(Vector3f);

        memcpy(
            &maxShadowDistance,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(float)
        );
        pos += sizeof(float);

        memcpy(
            &type,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(LightType)
        );
        pos += sizeof(LightType);

        memcpy(
            &enableShadows,
            reinterpret_cast<const uint8_t*>(pData) + pos,
            sizeof(uint8_t)
        );
        pos += sizeof(uint8_t);


        *ppLight = create_directional_light(
            entityID,
            direction,
            color,
            shadowProjectionMatrix,
            shadowViewMatrix,
            static_cast<bool>(enableShadows),
            maxShadowDistance,
            pScene,
            true
        );
    }
}
