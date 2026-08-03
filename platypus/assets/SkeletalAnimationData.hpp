#pragma once

#include "Asset.hpp"
#include "platypus/utils/AnimationDataUtils.hpp"


namespace platypus
{
    class SkeletalAnimationData : public Asset
    {
    private:
        KeyframeAnimationData _animationData;

    public:
        SkeletalAnimationData(
            size_t uuidPool,
            const KeyframeAnimationData& animationData
        );
        ~SkeletalAnimationData();

        // Returns matrix containing the interpolated translation and rotation
        // according to inputted time.
        // TODO: Add scaling
        // TODO: Optimize!
        Matrix4f getBoneMatrix(float time, int boneIndex) const;

        virtual void serialize(std::vector<char>& targetBuffer) const override;
        virtual size_t getSerializedSize() const override;

        inline const std::string& getName() const { return _animationData.name; }
        inline float getLength() const { return _animationData.length; }
    };
}
