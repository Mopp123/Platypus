#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"
#include <string>


namespace platypus
{
    enum class LightType : uint32_t
    {
        DIRECTIONAL_LIGHT,
        POINT_LIGHT,
        SPOT_LIGHT
    };

    std::string light_type_to_string(LightType type);

    constexpr size_t serialized_light_size =
        sizeof(ComponentType) +
        sizeof(Matrix4f) * 2 +
        sizeof(Vector3f) * 2 +
        sizeof(float) +
        sizeof(LightType) +
        sizeof(uint8_t);

    struct Light
    {
        Matrix4f shadowProjectionMatrix;
        Matrix4f shadowViewMatrix;
        Vector3f direction;
        Vector3f color;
        float maxShadowDistance;
        LightType type;
        uint8_t enableShadows;
    };

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        const Matrix4f& shadowProjectionMatrix,
        const Matrix4f& shadowViewMatrix,
        bool enableShadows,
        float maxShadowDistance,
        Scene* pScene = nullptr
    );

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        Scene* pScene = nullptr
    );

    std::vector<char> serialize(const Light* pLight);
    Light* deserialize(Scene* pScene, entityID_t entityID, size_t size, void* pData);
}
