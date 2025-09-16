#pragma once

#include "Asset.h"
#include "platypus/utils/AnimationDataUtils.h"


namespace platypus
{
    class SkeletalAnimationData : public Asset
    {
    private:
        Pose _bindPose;
        KeyframeAnimationData _animationData;

    public:
        SkeletalAnimationData(
            const Pose& bindPose,
            const KeyframeAnimationData& animationData
        );
        ~SkeletalAnimationData();

        // Returns matrix containing the interpolated translation and rotation
        // according to inputted time.
        // TODO: Add scaling
        // TODO: Optimize!
        Matrix4f getBoneMatrix(float time, int boneIndex) const;

        inline const std::string& getName() const { return _animationData.name; }
        inline float getLength() const { return _animationData.length; }
        inline const Pose& getBindPose() const { return _bindPose; }
        inline Pose* getBindPosePtr() { return &_bindPose; }
    };
}
