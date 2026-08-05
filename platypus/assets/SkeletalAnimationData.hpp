#pragma once

#include "Asset.hpp"
#include "platypus/utils/AnimationDataUtils.hpp"


namespace platypus
{
    class SkeletalAnimationData : public Asset
    {
    private:
        // NOTE: Previously had only KeyframeAnimationData member here
        float _length = 0.0f;
        // Indexing of these follows the bind pose's joints' indexing
        // which this animation is ment for.
        std::vector<JointAnimationData> _keyframeData;

    public:
        SkeletalAnimationData(
            size_t uuidPool,
            const KeyframeAnimationData& animationData
        );
        SkeletalAnimationData(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );
        ~SkeletalAnimationData();

        // Returns matrix containing the interpolated translation and rotation
        // according to inputted time.
        // TODO: Add scaling
        // TODO: Optimize!
        Matrix4f getBoneMatrix(float time, int boneIndex) const;

        virtual void serialize(std::vector<char>& targetBuffer) const override;
        virtual size_t getSerializedSize() const override;

        inline float getLength() const { return _length; }

    private:
        size_t getSerializedTranslationKeySize() const;
        size_t getSerializedRotationKeySize() const;
        size_t getSerializedJointAnimationDataSize(size_t jointIndex) const;
    };
}
