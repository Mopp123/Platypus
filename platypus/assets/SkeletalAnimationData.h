#pragma once

#include "Asset.h"
#include "platypus/utils/AnimationDataUtils.h"


namespace platypus
{
    class SkeletalAnimationData : public Asset
    {
    private:
        float _speed = 1.0f;
        Pose _bindPose;
        std::vector<Pose> _poses;

    public:
        SkeletalAnimationData(
            float speed,
            const Pose& bindPose,
            const std::vector<Pose>& poses
        );
        ~SkeletalAnimationData();

        inline float getSpeed() const { return _speed; }
        inline const Pose& getBindPose() const { return _bindPose; }
        inline const Pose& getPose(uint32_t index) const { return _poses[index]; }
        inline size_t getPoseCount() const { return _poses.size(); }
    };
}
