#pragma once

#include "platypus/ecs/Entity.h"
#include "platypus/utils/Maths.h"
#include "platypus/utils/ID.h"


namespace platypus
{
    enum class AnimationMode
    {
        ANIMATION_MODE_LOOP,
        ANIMATION_MODE_PLAY_ONCE
    };

    struct SkeletalAnimation
    {
        AnimationMode mode = AnimationMode::ANIMATION_MODE_LOOP;
        ID_t animationID = 0;
        float time = 0.0f;
        float length = 0.0f; // Total length of the anim in seconds
        bool stopped = false;

        size_t jointCount = 0;
        // Atm the matrices to throw to the shader
        Matrix4f jointMatrices[50];
    };

    // NOTE: ATM JUST TESTING WITH THIS
    struct SkeletonJoint
    {
        // Index in the skeleton hierarchy
        uint32_t jointIndex = 0;
    };

    SkeletalAnimation* create_skeletal_animation(
        entityID_t target,
        ID_t animationAssetID
    );

    SkeletonJoint* create_skeleton_joint(
        entityID_t target,
        uint32_t jointIndex
    );
}
