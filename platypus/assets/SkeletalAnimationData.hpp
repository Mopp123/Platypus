#pragma once

#include "Asset.hpp"
#include "platypus/utils/AnimationDataUtils.h"


namespace platypus
{
    class SkeletalAnimationData : public Asset
    {
    private:
        KeyframeAnimationData _animationData;

    public:
        SkeletalAnimationData(
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
    };
}
