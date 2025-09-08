#pragma once

#include "Asset.h"
#include "platypus/utils/AnimationDataUtils.h"


namespace platypus
{
    struct TranslationKey
    {
        float time = 0.0f;
        Vector3f translation;
    };

    struct RotationKey
    {
        float time = 0.0f;
        Quaternion rotation;
    };

    struct BoneAnimationData
    {
        std::vector<TranslationKey> translations;
        std::vector<RotationKey> rotations;
    };

    class SkeletalAnimationData : public Asset
    {
    private:
        float _speed = 1.0f;
        float _length = 0.0f; // Total length of the animation in seconds
        Pose _bindPose;

        // Each bone's keyframes in the same order as in bindPose
        std::vector<BoneAnimationData> _keyframes;

    public:
        SkeletalAnimationData(
            float length,
            const Pose& bindPose,
            const std::vector<BoneAnimationData>& keyframes
        );
        ~SkeletalAnimationData();

        // Returns matrix containing the interpolated translation and rotation
        // according to inputted time.
        // TODO: Add scaling
        // TODO: Optimize!
        Matrix4f getBoneMatrix(float time, int boneIndex) const;

        inline float getLength() const { return _length; }
        inline float getSpeed() const { return _speed; }
        inline const Pose& getBindPose() const { return _bindPose; }
        inline Pose* getBindPosePtr() { return &_bindPose; }
    };
}
