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


    class Skeleton : public Asset
    {
    private:
        std::vector<Joint> _joints;
        std::vector<std::vector<uint32_t>> _jointChildMapping;
        std::vector<UUID_t> _animationIDs;

    public:
        Skeleton(
            size_t uuidPool,
            const std::vector<Joint>& joints,
            const std::vector<std::vector<uint32_t>>& jointChildMapping,
            const std::vector<UUID_t>& animationIDs,
            const std::string& name = "",
            UUID_t id = NULL_UUID,
            bool persistent = false
        );
        Skeleton(
            AssetManager* pAssetManager,
            const std::vector<char>& targetBuffer,
            size_t bufferPos
        );
        ~Skeleton();

        // returns -1 if not found
        int32_t getAnimationIndex(const std::string& name) const;

        virtual void serialize(std::vector<char>& targetBuffer) const override;
        virtual size_t getSerializedSize() const override;

        inline const std::vector<Joint>& getJoints() const { return _joints; }
        inline const std::vector<std::vector<uint32_t>>& getJointChildMapping() const { return _jointChildMapping; }
        inline const Joint& getJoint(size_t index) const { return _joints[index]; }
        inline const size_t getJointCount() const { return _joints.size(); }
        inline const std::vector<UUID_t>& getAnimationIDs() const { return _animationIDs; }

    private:
        size_t getSerializedJointSize(size_t jointIndex) const;
    };

}
