#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"


namespace platypus
{
    struct DirectionalLight
    {
        Matrix4f shadowProjectionMatrix;
        Matrix4f viewMatrix;
        Vector3f direction;
        Vector3f color;
        bool enableShadows;
    };

    DirectionalLight* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color,
        const Matrix4f& shadowProjectionMatrix,
        const Matrix4f& viewMatrix,
        bool enableShadows
    );

    DirectionalLight* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color
    );
}
