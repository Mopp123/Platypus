#include "SkeletalAnimationData.h"

namespace platypus
{
    SkeletalAnimationData::SkeletalAnimationData(
        float speed,
        const Pose& bindPose,
        const std::vector<Pose>& poses
    ) :
        Asset(AssetType::ASSET_TYPE_SKELETAL_ANIMATION_DATA),
        _speed(speed),
        _bindPose(bindPose),
        _poses(poses)
    {}

    SkeletalAnimationData::~SkeletalAnimationData()
    {}
}
