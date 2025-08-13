#pragma once

#include "platypus/utils/Maths.h"
#include "platypus/ecs/Entity.h"
#include "platypus/utils/AnimationDataUtils.h"
#include <vector>

#define PLATYPUS_MAX_CHILD_ENTITIES 10

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

    struct Parent
    {
        entityID_t entityID = NULL_ENTITY_ID;
    };

    struct Children
    {
        size_t count = 0;
        entityID_t entityIDs[PLATYPUS_MAX_CHILD_ENTITIES];
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

    void set_transform_position(Transform* pTransform, const Vector3f& position, bool hasParent);
    void set_transform_rotation(Transform* pTransform, float pitch, float yaw, float roll, bool hasParent);
    void set_transform_rotation(Transform* pTransform, const Quaternion& rotation, bool hasParent);
    void rotate_transform(Transform* pTransform, float pAmount, float yAmount, float rAmount, bool hasParent);
    void set_transform_scale(Transform* pTransform, const Vector3f& scale, bool hasParent);

    // NOTE: None of these are tested, might point to wrong directions!
    Vector3f get_transform_forward(Transform* pTransform);
    Vector3f get_transform_up(Transform* pTransform);
    Vector3f get_transform_right(Transform* pTransform);

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

    void add_child(entityID_t target, entityID_t child);
}
