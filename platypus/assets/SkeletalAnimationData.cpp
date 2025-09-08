#include "SkeletalAnimationData.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    static float get_interpolation_amount(float currentTime, float prevTime, float nextTime)
    {
        float midWayLength = currentTime - prevTime;
        float framesDiff = nextTime - prevTime;
        return midWayLength / framesDiff;

        /*
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
        */
    }

    SkeletalAnimationData::SkeletalAnimationData(
        float length,
        const Pose& bindPose,
        const std::vector<BoneAnimationData>& keyframes
    ) :
        Asset(AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA),
        _length(length),
        _bindPose(bindPose),
        _keyframes(keyframes)
    {}

    SkeletalAnimationData::~SkeletalAnimationData()
    {}

    Matrix4f SkeletalAnimationData::getBoneMatrix(float time, int boneIndex) const
    {
        // Get interpolated translation
        const std::vector<TranslationKey>& translationKeys = _keyframes[boneIndex].translations;
        TranslationKey currentTranslationKey = translationKeys[0];
        TranslationKey nextTranslationKey = translationKeys[0];
        for (size_t i = 0; i < translationKeys.size() - 1; ++i)
        {
            if (time < translationKeys[i + 1].time)
            {
                currentTranslationKey = translationKeys[i];
                nextTranslationKey = translationKeys[i + 1];
                break;
            }
        }
        Vector3f interpolatedTranslation = currentTranslationKey.translation;
        float translationInterpolationAmount = get_interpolation_amount(
            time,
            currentTranslationKey.time,
            nextTranslationKey.time
        );
        if (translationInterpolationAmount <= 1.0f)
        {
            interpolatedTranslation = interpolatedTranslation.lerp(
                nextTranslationKey.translation,
                translationInterpolationAmount
            );
        }

        // Get interpolated rotation
        const std::vector<RotationKey>& rotationKeys = _keyframes[boneIndex].rotations;
        RotationKey currentRotationKey = rotationKeys[0];
        RotationKey nextRotationKey = rotationKeys[0];
        for (size_t i = 0; i < rotationKeys.size() - 1; ++i)
        {
            if (time < rotationKeys[i + 1].time)
            {
                currentRotationKey = rotationKeys[i];
                nextRotationKey = rotationKeys[i + 1];
                break;
            }
        }
        Quaternion interpolatedRotation = currentRotationKey.rotation;
        float rotationInterpolationAmount = get_interpolation_amount(
            time,
            currentRotationKey.time,
            nextRotationKey.time
        );
        if (rotationInterpolationAmount <= 1.0f)
        {
            interpolatedRotation = interpolatedRotation.slerp(
                nextRotationKey.rotation,
                rotationInterpolationAmount
            );
        }

        // Combine and return
        Matrix4f translationMatrix(1.0f);
        translationMatrix[0 + 3 * 4] = interpolatedTranslation.x;
        translationMatrix[1 + 3 * 4] = interpolatedTranslation.y;
        translationMatrix[2 + 3 * 4] = interpolatedTranslation.z;
        return translationMatrix * interpolatedRotation.toRotationMatrix();
    }
}
