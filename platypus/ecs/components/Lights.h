#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"


namespace platypus
{
    struct DirectionalLight
    {
        Vector3f direction;
        Vector3f color;
    };

    DirectionalLight* create_directional_light(
        entityID_t target,
        const Vector3f& direction,
        const Vector3f& color
    );
}
