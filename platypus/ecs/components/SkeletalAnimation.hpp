#pragma once

#include "platypus/ecs/Entity.hpp"
#include "platypus/utils/Maths.hpp"
#include "platypus/utils/ID.hpp"
#include "platypus/core/Scene.hpp"


namespace platypus
{
    enum class AnimationMode : uint32_t
    {
        ANIMATION_MODE_LOOP,
        ANIMATION_MODE_PLAY_ONCE
    };

    constexpr size_t skeletal_animation_max_joints = 50;

    constexpr size_t serialized_skeletal_animation_size =
        sizeof(ComponentType) +
        sizeof(AnimationMode) +
        sizeof(ID_t) +
        sizeof(float) * 2 +
        sizeof(uint8_t) +
        sizeof(Matrix4f) * skeletal_animation_max_joints;

    constexpr size_t serialized_skeleton_joint_size =
        sizeof(ComponentType) +
        sizeof(uint32_t);

    struct SkeletalAnimation
    {
        AnimationMode mode = AnimationMode::ANIMATION_MODE_LOOP;
        ID_t animationID = 0;
        float time = 0.0f;
        float length = 0.0f; // Total length of the anim in seconds
        uint8_t stopped = 0;

        // Atm the matrices to throw to the shader
        // NOTE: Currently the joint count used, should be fetched from the Mesh asset!
        Matrix4f jointMatrices[skeletal_animation_max_joints];
    };

    // NOTE: ATM JUST TESTING WITH THIS
    struct SkeletonJoint
    {
        // Index in the skeleton hierarchy
        uint32_t jointIndex = 0;
    };

    SkeletalAnimation* create_skeletal_animation(
        entityID_t target,
        ID_t animationAssetID,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    SkeletonJoint* create_skeleton_joint(
        entityID_t target,
        uint32_t jointIndex,
        Scene* pScene = nullptr,
        bool useExplicitComponentMask = false
    );

    std::vector<char> serialize(const SkeletalAnimation* pSkeletalAnimation);
    std::vector<char> serialize(const SkeletonJoint* pSkeletonJoint);
    void deserialize(
        Scene* pScene,
        SkeletalAnimation** ppSkeletalAnimation,
        entityID_t entityID,
        size_t dataSize,
        void* pData
    );
    void deserialize(
        Scene* pScene,
        SkeletonJoint** ppSkeletonJoint,
        entityID_t entityID,
        size_t dataSize,
        void* pData
    );
}
