#pragma once

#include "platypus/ecs/Entity.h"
#include "platypus/utils/AnimationDataUtils.h"
#include "platypus/utils/ID.h"
#include <vector>


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
        // Indices to animation asset's poses
        uint32_t currentPose = 0;
        uint32_t nextPose = 1;
        // Progression between current and next pose
        float progress = 0.0f;
        float speed = 1.0f;

        bool stopped = false;

        size_t jointCount = 0;
        Matrix4f jointMatrices[50];
    };

    SkeletalAnimation* create_skeletal_animation(
        entityID_t target,
        ID_t animationAssetID,
        float speed
    );
}
