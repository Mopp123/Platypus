#pragma once

#include "platypus/utils/SkeletalAnimationData.h"

#include <tiny_gltf.h>
#include <unordered_map>


namespace platypus
{
    void add_gltf_joint(
        const tinygltf::Model& gltfModel,
        Pose& pose,
        int parentJointPoseIndex, // index to pose struct's parent joint. NOT glTF node index!
        int jointNodeIndex,
        std::unordered_map<int, int>& outNodeJointMapping
    );
}
