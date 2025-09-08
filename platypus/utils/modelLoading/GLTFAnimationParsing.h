#pragma once

#include "platypus/utils/AnimationDataUtils.h"
#include "platypus/assets/SkeletalAnimationData.h"
#include <unordered_map>


// Nasty shit to be able to use here...
namespace tinygltf
{
    class Model;
    struct Mesh;
}

namespace platypus
{
    void add_gltf_joint(
        const tinygltf::Model& gltfModel,
        Pose& pose,
        int parentJointPoseIndex, // index to pose struct's parent joint. NOT glTF node index!
        int jointNodeIndex,
        std::unordered_map<int, int>& outNodeJointMapping
    );

    // First = total length of the animation in seconds
    // Second = animation data
    std::pair<float, std::vector<BoneAnimationData>> load_gltf_anim_poses(
        tinygltf::Model& gltfModel,
        const Pose& bindPose,
        std::unordered_map<int, int> nodePoseJointMapping
    );

    Pose load_gltf_joints(
        const tinygltf::Model& gltfModel,
        size_t gltfSkinIndex,
        std::unordered_map<int, int>& outNodeJointMapping // Mapping from gltf joint node index to our pose struct's joint index
    );
}
