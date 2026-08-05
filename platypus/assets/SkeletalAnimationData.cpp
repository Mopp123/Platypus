#include "SkeletalAnimationData.hpp"
#include "AssetManager.hpp"
#include "platypus/core/Debug.hpp"


namespace platypus
{
    static float get_interpolation_amount(float currentTime, float prevTime, float nextTime)
    {
        float midWayLength = currentTime - prevTime;
        float framesDiff = nextTime - prevTime;
        return midWayLength / framesDiff;
    }


    Skeleton::Skeleton(
        size_t uuidPool,
        const std::vector<Joint>& joints,
        const std::vector<std::vector<uint32_t>>& jointChildMapping,
        const std::string& name,
        UUID_t id,
        bool persistent
    ) :
        Asset(uuidPool, AssetType::ASSET_TYPE_SKELETON, name, id, persistent),
        _joints(joints),
        _jointChildMapping(jointChildMapping)
    {
    }

    Skeleton::Skeleton(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    ) :
        Asset(pAssetManager, targetBuffer, bufferPos)
    {
        const size_t baseAssetSerializedSize = getSerializedBaseSize();
        PLATYPUS_ASSERT(bufferPos + baseAssetSerializedSize < targetBuffer.size());

        const char* pBuf = targetBuffer.data() + bufferPos + baseAssetSerializedSize;

        uint32_t jointCountU32 = 0;
        memcpy(&jointCountU32, pBuf, sizeof(uint32_t));
        size_t pos = sizeof(uint32_t);
        const size_t jointCount = static_cast<const uint32_t>(jointCountU32);

        _joints.resize(jointCount);
        _jointChildMapping.resize(jointCount);

        for (size_t i = 0; i < jointCount; ++i)
        {
            Joint& joint = _joints[i];
            memcpy(&joint.translation, pBuf + pos, sizeof(Vector3f));
            pos += sizeof(Vector3f);

            memcpy(&joint.rotation, pBuf + pos, sizeof(Quaternion));
            pos += sizeof(Quaternion);

            memcpy(&joint.scale, pBuf + pos, sizeof(Vector3f));
            pos += sizeof(Vector3f);

            memcpy(&joint.matrix, pBuf + pos, sizeof(Matrix4f));
            pos += sizeof(Matrix4f);

            memcpy(&joint.inverseMatrix, pBuf + pos, sizeof(Matrix4f));
            pos += sizeof(Matrix4f);

            uint32_t jointNameSizeU32 = 0;
            memcpy(&jointNameSizeU32, pBuf + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);
            const size_t jointNameSize = static_cast<const size_t>(jointNameSizeU32);

            uint32_t childJointCountU32 = 0;
            memcpy(&childJointCountU32, pBuf + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);
            const size_t childJointCount = static_cast<const size_t>(childJointCountU32);

            char* pJointNameData = new char[jointNameSize];
            memcpy(pJointNameData, pBuf + pos, jointNameSize);
            pos += jointNameSize;
            joint.name = std::string(pJointNameData, jointNameSize);
            delete[] pJointNameData;

            std::vector<uint32_t> childIndices(childJointCount);
            if (childJointCount > 0)
            {
                memcpy(childIndices.data(), pBuf + pos, sizeof(uint32_t) * childJointCount);
                pos += sizeof(uint32_t) * childJointCount;
            }
            _jointChildMapping[i] = childIndices;
        }
    }

    Skeleton::~Skeleton()
    {
    }

    /*
        Serialized format:
            Asset serialized base data
            uint32_t jointCount
            Joint jointData[jointCount]
    */
    void Skeleton::serialize(std::vector<char>& targetBuffer) const
    {
        const size_t prevSize = targetBuffer.size();
        const size_t serializedSize = getSerializedSize();
        targetBuffer.resize(serializedSize);

        char* pBuf = targetBuffer.data() + prevSize;
        serializeBase(pBuf);

        size_t pos = getSerializedBaseSize();

        const size_t jointCount = _joints.size();
        const uint32_t jointCountU32 = static_cast<const uint32_t>(jointCount);
        memcpy(pBuf + pos, &jointCountU32, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
        {
            const Joint& joint = _joints[jointIndex];
            memcpy(pBuf + pos, &joint.translation, sizeof(Vector3f));
            pos += sizeof(Vector3f);

            memcpy(pBuf + pos, &joint.rotation, sizeof(Quaternion));
            pos += sizeof(Quaternion);

            memcpy(pBuf + pos, &joint.scale, sizeof(Vector3f));
            pos += sizeof(Vector3f);

            memcpy(pBuf + pos, &joint.matrix, sizeof(Matrix4f));
            pos += sizeof(Matrix4f);

            memcpy(pBuf + pos, &joint.inverseMatrix, sizeof(Matrix4f));
            pos += sizeof(Matrix4f);

            const uint32_t nameSizeU32 = static_cast<const uint32_t>(joint.name.size());
            memcpy(pBuf + pos, &nameSizeU32, sizeof(uint32_t));
            pos += sizeof(uint32_t);

            PLATYPUS_ASSERT(jointIndex < _jointChildMapping.size());
            const std::vector<uint32_t>& childJointIndices = _jointChildMapping[jointIndex];
            const size_t childJointCount = childJointIndices.size();
            const uint32_t childJointCountU32 = static_cast<const uint32_t>(childJointCount);
            memcpy(pBuf + pos, &childJointCountU32, sizeof(uint32_t));
            pos += sizeof(uint32_t);

            memcpy(pBuf + pos, joint.name.data(), joint.name.size());
            pos += joint.name.size();

            if (childJointCount > 0)
            {
                memcpy(pBuf + pos, childJointIndices.data(), sizeof(uint32_t) * childJointCount);
                pos += sizeof(uint32_t) * childJointCount;
            }
        }
    }

    size_t Skeleton::getSerializedSize() const
    {
        size_t serializedJointsSize = 0;
        for (size_t i = 0; i < _joints.size(); ++i)
            serializedJointsSize += getSerializedJointSize(i);

        return getSerializedBaseSize() +
            sizeof(uint32_t) + // joint count
            serializedJointsSize;
    }

    size_t Skeleton::getSerializedJointSize(size_t jointIndex) const
    {
        PLATYPUS_ASSERT(jointIndex < _joints.size());
        size_t childIndicesSize = 0;
        if (jointIndex < _jointChildMapping.size())
            childIndicesSize = _jointChildMapping[jointIndex].size();

        return sizeof(Vector3f) + // translation
            sizeof(Quaternion) + // rotation
            sizeof(Vector3f) + // scale
            sizeof(Matrix4f) + // joint matrix
            sizeof(Matrix4f) + // inverse matrix
            sizeof(uint32_t) + // joint name size
            sizeof(uint32_t) + // child indices count
            _joints[jointIndex].name.size() +
            childIndicesSize;
    }


    SkeletalAnimationData::SkeletalAnimationData(
        size_t uuidPool,
        const KeyframeAnimationData& animationData
    ) :
        Asset(uuidPool, AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA, animationData.name, NULL_UUID, false),
        _length(animationData.length),
        _keyframeData(animationData.keyframes)
    {

    }

    SkeletalAnimationData::SkeletalAnimationData(
        AssetManager* pAssetManager,
        const std::vector<char>& targetBuffer,
        size_t bufferPos
    ) :
        Asset(pAssetManager, targetBuffer, bufferPos)
    {
        const size_t serializedBaseSize = getSerializedBaseSize();
        PLATYPUS_ASSERT(bufferPos + serializedBaseSize < targetBuffer.size());

        const char* pBuf = targetBuffer.data() + bufferPos;
        size_t pos = serializedBaseSize;

        memcpy(&_length, pBuf + pos, sizeof(float));
        pos += sizeof(float);

        uint32_t jointCountU32 = 0;
        memcpy(&jointCountU32, pBuf + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);
        const size_t jointCount = static_cast<const size_t>(jointCountU32);
        _keyframeData.resize(jointCount);
        for (uint32_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
        {
            JointAnimationData& jointAnimData = _keyframeData[jointIndex];
            uint32_t translationKeyCountU32 = 0;
            uint32_t rotationKeyCountU32 = 0;
            memcpy(&translationKeyCountU32, pBuf + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);
            memcpy(&rotationKeyCountU32, pBuf + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);

            const size_t translationKeyCount = static_cast<const size_t>(translationKeyCountU32);
            const size_t rotationKeyCount = static_cast<const size_t>(rotationKeyCountU32);

            jointAnimData.translations.resize(translationKeyCount);
            jointAnimData.rotations.resize(rotationKeyCount);

            for (size_t translationKeyIndex = 0; translationKeyIndex < translationKeyCount; ++translationKeyIndex)
            {
                memcpy(
                    &jointAnimData.translations[translationKeyIndex].time,
                    pBuf + pos,
                    sizeof(float)
                );
                pos += sizeof(float);
                memcpy(
                    &jointAnimData.translations[translationKeyIndex].translation,
                    pBuf + pos,
                    sizeof(Vector3f)
                );
                pos += sizeof(Vector3f);
            }

            for (size_t rotationKeyIndex = 0; rotationKeyIndex < rotationKeyCount; ++rotationKeyIndex)
            {
                memcpy(
                    &jointAnimData.rotations[rotationKeyIndex].time,
                    pBuf + pos,
                    sizeof(float)
                );
                pos += sizeof(float);
                memcpy(
                    &jointAnimData.rotations[rotationKeyIndex].rotation,
                    pBuf + pos,
                    sizeof(Quaternion)
                );
                pos += sizeof(Quaternion);
            }
        }

        pAssetManager->addExternalAsset(this);
        if (_persistent)
            pAssetManager->makePersistent(this);
    }

    SkeletalAnimationData::~SkeletalAnimationData()
    {}

    Matrix4f SkeletalAnimationData::getBoneMatrix(float time, int boneIndex) const
    {
        // Get interpolated translation
        const std::vector<TranslationKey>& translationKeys = _keyframeData[boneIndex].translations;
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
        const std::vector<RotationKey>& rotationKeys = _keyframeData[boneIndex].rotations;
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

    /*
        Serialized format:
            Asset serialized base data
            float animLength
            uint32_t jointCount
            JointAnimationData jointAnimData[jointCount]
    */
    void SkeletalAnimationData::serialize(std::vector<char>& targetBuffer) const
    {
        const size_t prevSize = targetBuffer.size();
        const size_t serializedSize = getSerializedSize();
        targetBuffer.resize(prevSize + serializedSize);

        char* pBuf = targetBuffer.data() + prevSize;
        serializeBase(pBuf);
        size_t pos = getSerializedBaseSize();

        memcpy(pBuf + pos, &_length, sizeof(float));
        pos += sizeof(float);

        const size_t jointCount = _keyframeData.size();
        const uint32_t jointCountU32 = static_cast<const uint32_t>(jointCount);
        memcpy(pBuf + pos, &jointCountU32, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        for (size_t i = 0; i < jointCount; ++i)
        {
            const JointAnimationData& jointAnimData = _keyframeData[i];
            const size_t translationKeyCount = jointAnimData.translations.size();
            const size_t rotationKeyCount = jointAnimData.rotations.size();
            const uint32_t translationKeyCountU32 = static_cast<const uint32_t>(translationKeyCount);
            const uint32_t rotationKeyCountU32 = static_cast<const uint32_t>(rotationKeyCount);
            memcpy(pBuf + pos, &translationKeyCountU32, sizeof(uint32_t));
            pos += sizeof(uint32_t);
            memcpy(pBuf + pos, &rotationKeyCountU32, sizeof(uint32_t));
            pos += sizeof(uint32_t);

            for (size_t translationKeyIndex = 0; translationKeyIndex < translationKeyCount; ++translationKeyIndex)
            {
                const TranslationKey& translationKey = jointAnimData.translations[translationKeyIndex];
                memcpy(pBuf + pos, &translationKey.time, sizeof(float));
                pos += sizeof(float);
                memcpy(pBuf + pos, &translationKey.translation, sizeof(Vector3f));
                pos += sizeof(Vector3f);
            }

            for (size_t rotationKeyIndex = 0; rotationKeyIndex < rotationKeyCount; ++rotationKeyIndex)
            {
                const RotationKey& rotationKey = jointAnimData.rotations[rotationKeyIndex];
                memcpy(pBuf + pos, &rotationKey.time, sizeof(float));
                pos += sizeof(float);
                memcpy(pBuf + pos, &rotationKey.rotation, sizeof(Quaternion));
                pos += sizeof(Quaternion);
            }
        }
    }

    size_t SkeletalAnimationData::getSerializedSize() const
    {
        size_t combinedJointAnimationDataSize = 0;
        for (size_t i = 0; i < _keyframeData.size(); ++i)
            combinedJointAnimationDataSize += getSerializedJointAnimationDataSize(i);

        return getSerializedBaseSize() +
            sizeof(float) + // anim length
            sizeof(uint32_t) + // joint count
            combinedJointAnimationDataSize;
    }

    size_t SkeletalAnimationData::getSerializedTranslationKeySize() const
    {
        return sizeof(float) + // time
            sizeof(Vector3f); // translation
    }

    size_t SkeletalAnimationData::getSerializedRotationKeySize() const
    {
        return sizeof(float) + // time
            sizeof(Quaternion); // rotation
    }

    // TODO: Add scale keys
    size_t SkeletalAnimationData::getSerializedJointAnimationDataSize(size_t jointIndex) const
    {
        PLATYPUS_ASSERT(jointIndex < _keyframeData.size());
        return sizeof(uint32_t) + // translation key count
            sizeof(uint32_t) + // rotation key count
            getSerializedTranslationKeySize() * _keyframeData[jointIndex].translations.size() +
            getSerializedRotationKeySize() * _keyframeData[jointIndex].rotations.size();
    }
}
