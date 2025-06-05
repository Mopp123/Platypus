#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"
#include "platypus/utils/AnimationDataUtils.h"
#include <vector>


namespace platypus
{
    struct Transform
    {
        Matrix4f localMatrix = Matrix4f(1.0f);
        Matrix4f globalMatrix = Matrix4f(1.0f);
    };

    struct GUITransform
    {
        Vector2f position;
        Vector2f scale;
    };


    Transform* create_transform(
        entityID_t target,
        Matrix4f matrix
    );

    Transform* create_transform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale
    );

    // Creates Transform hierarchy for joints
    std::vector<entityID_t> create_skeleton(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping
    );

    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale
    );
}
