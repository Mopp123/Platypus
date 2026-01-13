#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"


namespace platypus
{
    struct Camera
    {
        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
        Matrix4f orthographicProjectionMatrix = Matrix4f(1.0f);
        float aspectRatio;
        float fov;
        float zNear;
        float zFar;
    };

    Camera* create_camera(
        entityID_t target,
        float aspectRatio,
        float fov,
        float zNear,
        float zFar,
        const Matrix4f& orthographicProjectionMatrix // Used for 2D rendering stuff
    );
}
