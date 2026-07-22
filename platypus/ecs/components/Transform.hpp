#pragma once

#include "platypus/utils/Maths.hpp"
#include "platypus/ecs/Entity.hpp"
#include "platypus/utils/AnimationDataUtils.hpp"
#include "platypus/core/Scene.hpp"
#include <vector>

// This is quite low...
// TODO: Figure out better way to set limits like this..
#define PLATYPUS_MAX_CHILD_ENTITIES 10

namespace platypus
{
    constexpr size_t serialized_transform_size =
        sizeof(ComponentType) +
        sizeof(Matrix4f) * 2;

    constexpr size_t serialized_gui_transform_size =
        sizeof(ComponentType) +
        sizeof(Vector2f) * 2;

    // NOTE: IMPORTANT!
    // serialized Parent and Children components has their target entities as UUIDs NOT entityID_t!!!
    //  The actual entityID_ts gets eventually resolved in the deserialization process.
    constexpr size_t serialized_parent_size =
        sizeof(ComponentType) +
        sizeof(UUID_t);

    constexpr size_t serialized_children_size =
        sizeof(ComponentType) +
        sizeof(uint32_t) +
        sizeof(UUID_t) * PLATYPUS_MAX_CHILD_ENTITIES;

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
        uint32_t count = 0;
        entityID_t entityIDs[PLATYPUS_MAX_CHILD_ENTITIES];
    };


    Transform* create_transform(
        entityID_t target,
        Matrix4f matrix,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    Transform* create_transform(
        entityID_t target,
        const Vector3f& position,
        const Quaternion& rotation,
        const Vector3f& scale,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    // NOTE: WARNING! Below might actually be fucked atm!?
    void set_transform_position(Transform* pTransform, const Vector3f& position, bool hasParent);
    void set_transform_rotation(Transform* pTransform, float pitch, float yaw, float roll, bool hasParent);
    void set_transform_rotation(Transform* pTransform, const Quaternion& rotation, bool hasParent);
    void rotate_transform(Transform* pTransform, float pAmount, float yAmount, float rAmount, bool hasParent);
    void set_transform_scale(Transform* pTransform, const Vector3f& scale, bool hasParent);

    Vector3f get_transform_position(const Transform* pTransform, bool hasParent);
    Vector3f get_transform_euler_rotation(const Transform* pTransform, bool hasParent);
    // NOTE: NOT TESTED -> might be fucked!
    Vector3f get_transform_scale(const Transform* pTransform, bool hasParent);

    // NOTE: None of these are tested, might point to wrong directions!
    Vector3f get_transform_forward(Transform* pTransform);
    Vector3f get_transform_up(Transform* pTransform);
    Vector3f get_transform_right(Transform* pTransform);

    Matrix4f get_global_rotation_matrix(const Transform* pTransform);

    // Creates Transform hierarchy for joints and returns the root joint entity
    std::vector<entityID_t> create_skeleton(
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        Scene* pScene = nullptr
    );

    GUITransform* create_gui_transform(
        entityID_t target,
        const Vector2f position,
        const Vector2f scale,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    Parent* create_parent(
        entityID_t target,
        entityID_t parentID,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );
    Children* create_children(
        entityID_t target,
        std::vector<entityID_t> childIDs,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    void add_child(entityID_t target, entityID_t child, Scene* pScene = nullptr);
    void remove_child(entityID_t target, entityID_t child, Scene* pScene = nullptr);
    // Moves all children after freedPosition, so that all child IDs are contiguous.
    // Calling this assumes that there was only a single gap in child entity IDs!
    void pack_children(Children* pChildren, size_t freedPosition);

    std::vector<char> serialize(const Transform* pTransform);
    std::vector<char> serialize(const GUITransform* pTransform);
    std::vector<char> serialize(const Parent* pParent);
    std::vector<char> serialize(const Children* pChildren);

    void deserialize(
        Scene* pScene,
        Transform** ppTransform,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );

    void deserialize(
        Scene* pScene,
        GUITransform** ppTransform,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );

    void deserialize(
        Scene* pScene,
        Parent** ppParent,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );

    void deserialize(
        Scene* pScene,
        Children** ppChildren,
        entityID_t entityID,
        size_t dataSize,
        const void* pData
    );
}
