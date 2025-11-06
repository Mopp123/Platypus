#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"
#include <string>


namespace platypus
{
    enum class LightType
    {
        DIRECTIONAL_LIGHT,
        POINT_LIGHT,
        SPOT_LIGHT
    };

    std::string light_type_to_string(LightType type);

    struct Light
    {
        Matrix4f shadowProjectionMatrix;
        Matrix4f shadowViewMatrix;
        Vector3f direction;
        Vector3f color;
        float maxShadowDistance;
        LightType type;
        bool enableShadows;
    };

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        const Matrix4f& shadowProjectionMatrix,
        const Matrix4f& shadowViewMatrix,
        bool enableShadows,
        float maxShadowDistance
    );

    Light* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color
    );
}
