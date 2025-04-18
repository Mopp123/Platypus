#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"


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
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale
    );

    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale
    );
}
