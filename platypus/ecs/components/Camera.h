#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"


namespace platypus
{
    struct Camera
    {
        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f orthographicProjectionMatrix = Matrix4f(1.0f);
    };

    Camera* create_camera(
        entityID_t target,
        const Matrix4f& perspectiveProjectionMatrix,
        const Matrix4f& orthographicProjectionMatrix
    );
}
