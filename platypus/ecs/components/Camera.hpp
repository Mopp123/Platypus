#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "platypus/core/Scene.hpp"


namespace platypus
{
    constexpr size_t serialized_camera_size =
        sizeof(ComponentType) +
        sizeof(Matrix4f) * 2 +
        sizeof(float) * 4;

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
        const Matrix4f& orthographicProjectionMatrix, // Used for 2D rendering stuff
        Scene* pScene = nullptr
    );

    std::vector<char> serialize(const Camera* pCamera);
}
