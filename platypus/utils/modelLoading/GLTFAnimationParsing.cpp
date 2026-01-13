#include "GLTFAnimationParsing.h"
#include "GLTFFileUtils.h"
#include "platypus/Common.h"
#include "platypus/core/Debug.hpp"

#include <tiny_gltf.h>
#include <unordered_map>
#include <set>
#include <algorithm>


namespace platypus
{
    void add_gltf_joint(
        const tinygltf::Model& gltfModel,
        Pose& pose,
        int parentJointPoseIndex, // index to pose struct's parent joint. NOT glTF node index!
        int jointNodeIndex,
        std::unordered_map<int, int>& outNodeJointMapping
    )
    {
        const tinygltf::Node& node = gltfModel.nodes[jointNodeIndex];
        Matrix4f jointMatrix = to_engine_matrix(node.matrix);

        Vector3f translation;
        Quaternion rotation(0, 0, 0, 1);
        Vector3f scale(1.0f, 1.0f, 1.0f);
        if (node.translation.size() == 3)
        {
            translation = Vector3f(
                node.translation[0],
                node.translation[1],
                node.translation[2]
            );
        }
        if (node.rotation.size() == 4)
        {
            rotation = Quaternion(
                node.rotation[0],
                node.rotation[1],
                node.rotation[2],
                node.rotation[3]
            );
            rotation = rotation.normalize();
        }
        if (node.scale.size() == 3)
        {
            scale = Vector3f(
                node.scale[0],
                node.scale[1],
                node.scale[2]
            );
        }

        if (jointMatrix == Matrix4f(0.0f))
        {
            Matrix4f translationMatrix(1.0f);
            translationMatrix[0 + 3 * 4] = translation.x;
            translationMatrix[1 + 3 * 4] = translation.y;
            translationMatrix[2 + 3 * 4] = translation.z;

            Matrix4f rotationMatrix = rotation.toRotationMatrix();

            Matrix4f scaleMatrix(1.0f);
            translationMatrix[0 + 0 * 4] = scale.x;
            translationMatrix[1 + 1 * 4] = scale.y;
            translationMatrix[2 + 2 * 4] = scale.z;

            jointMatrix = translationMatrix * rotationMatrix * scaleMatrix;
        }

        Joint joint = {
            translation,
            rotation,
            scale,
            jointMatrix,
            Matrix4f(1.0f), // Inverse bind matrices are taken from gltf buffer
            node.name
        };
        const std::vector<int>& children = node.children;
        pose.joints.push_back(joint);
        pose.jointChildMapping.push_back({});
        const int currentJointPoseIndex = pose.joints.size() - 1;
        outNodeJointMapping[jointNodeIndex] = currentJointPoseIndex;
        if (parentJointPoseIndex != -1)
            pose.jointChildMapping[parentJointPoseIndex].push_back(currentJointPoseIndex);

        for (int childNodeIndex : children)
        {
            add_gltf_joint(
                gltfModel,
                pose,
                currentJointPoseIndex,
                childNodeIndex,
                outNodeJointMapping
            );
        }
    }


    static std::string float_vector_to_string(const std::vector<float>& v)
    {
        std::string s;
        for(size_t i = 0; i < v.size(); ++i)
        {
            s += std::to_string(v[i]);
            if (i + 1 < v.size())
                s += ", ";
        }
        return s;
    }

    static std::string vec3_vector_to_string(const std::vector<Vector3f>& v)
    {
        std::string s;
        for(size_t i = 0; i < v.size(); ++i)
        {
            s += v[i].toString();
            if (i + 1 < v.size())
                s += "] ";
        }
        return s;
    }

    static std::string quat_vector_to_string(const std::vector<Quaternion>& v)
    {
        std::string s;
        for(size_t i = 0; i < v.size(); ++i)
        {
            s += v[i].toString();
            if (i + 1 < v.size())
                s += ", ";
        }
        return s;
    }


    std::vector<KeyframeAnimationData> load_gltf_animations(
        tinygltf::Model& gltfModel,
        const std::unordered_map<int, int>& nodePoseJointMapping
    )
    {
        std::vector<KeyframeAnimationData> animations;
        animations.resize(gltfModel.animations.size());
        static const std::set<std::string> allowedInterpolations = { "LINEAR" };

        for (size_t animationIndex = 0; animationIndex < gltfModel.animations.size(); ++animationIndex)
        {
            tinygltf::Animation& gltfAnimation = gltfModel.animations[animationIndex];
            animations[animationIndex].keyframes.resize(nodePoseJointMapping.size());
            float maxKeyframeTime = 0.0f;
            for (tinygltf::AnimationChannel& channel : gltfAnimation.channels)
            {
                int targetNode = channel.target_node;

                std::unordered_map<int, int>::const_iterator poseJointMappingIt = nodePoseJointMapping.find(targetNode);
                if (poseJointMappingIt == nodePoseJointMapping.end())
                {
                    Debug::log(
                        "@load_gltf_animations "
                        "Failed to find joint using glTF node(index: " + std::to_string(targetNode) + ", name: " + gltfModel.nodes[targetNode].name + ")",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return { };
                }
                int targetPoseJoint = poseJointMappingIt->second;

                const tinygltf::AnimationSampler& sampler = gltfAnimation.samplers[channel.sampler];
                if (allowedInterpolations.find(sampler.interpolation) == allowedInterpolations.end())
                {
                    std::string allowedStr;
                    size_t allowedIndex = 0;
                    for (const std::string& s : allowedInterpolations)
                    {
                        allowedStr += s;
                        if (allowedIndex + 1 < allowedInterpolations.size())
                            allowedStr += ", ";

                        ++allowedIndex;
                    }

                    Debug::log(
                        "@load_gltf_animations "
                        "Animation channel targeting node(index: " + std::to_string(targetNode) + ", name: " + gltfModel.nodes[targetNode].name + ") "
                        "uses " + sampler.interpolation + " interpolation. "
                        "Currently supported interpolations: " + allowedStr + " (spherical linear for rotations).",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return { };
                }

                // Get keyframe times
                const tinygltf::Accessor& keyframeAccessor = gltfModel.accessors[sampler.input];
                const tinygltf::BufferView& keyframeBufferView = gltfModel.bufferViews[keyframeAccessor.bufferView];
                const tinygltf::Buffer& keyframeBuffer = gltfModel.buffers[keyframeBufferView.buffer];
                PE_byte* pKeyframeData = (PE_byte*)(keyframeBuffer.data.data()) + keyframeBufferView.byteOffset + keyframeAccessor.byteOffset;
                std::vector<float> keyframes(keyframeAccessor.count);
                memcpy((void*)keyframes.data(), pKeyframeData, sizeof(float) * keyframeAccessor.count);

                // Get anim data of this channel
                const tinygltf::Accessor& animDataAccessor = gltfModel.accessors[sampler.output];
                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[animDataAccessor.bufferView];
                const tinygltf::Buffer& animDataBuffer = gltfModel.buffers[bufferView.buffer];

                std::string accessorType = gltf_accessor_type_to_string(
                    animDataAccessor.type
                );

                std::string accessorComponentType = gltf_accessor_component_type_to_string(
                    animDataAccessor.componentType
                );

                PE_byte* pAnimData = (PE_byte*)&animDataBuffer.data[animDataAccessor.byteOffset + bufferView.byteOffset];

                const std::string& path = channel.target_path;
                // TODO: Scale
                if (path == "translation")
                {
                    std::vector<Vector3f> data(animDataAccessor.count);
                    memcpy((void*)data.data(), pAnimData, animDataAccessor.count * sizeof(Vector3f));
                    std::string dataStr = vec3_vector_to_string(data);

                    if (keyframes.size() != data.size())
                    {
                        Debug::log(
                            "@load_gltf_animations "
                            "Mismatch in keyframe and joint translation counts!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        return { };
                    }
                    for (size_t i = 0; i < data.size(); ++i)
                    {
                        float time = keyframes[i];
                        maxKeyframeTime = std::max(maxKeyframeTime, time);
                        animations[animationIndex].keyframes[targetPoseJoint].translations.push_back(
                            { time, data[i] }
                        );
                    }
                }
                else if (path == "rotation")
                {
                    std::vector<Quaternion> data(animDataAccessor.count);
                    memcpy((void*)data.data(), pAnimData, animDataAccessor.count * sizeof(Quaternion));
                    std::string dataStr = quat_vector_to_string(data);

                    if (keyframes.size() != data.size())
                    {
                        Debug::log(
                            "@load_gltf_animations_NEW "
                            "Mismatch in keyframe and bone rotation counts!",
                            Debug::MessageType::PLATYPUS_ERROR
                        );
                        PLATYPUS_ASSERT(false);
                        return { };
                    }
                    for (size_t i = 0; i < data.size(); ++i)
                    {
                        float time = keyframes[i];
                        maxKeyframeTime = std::max(maxKeyframeTime, time);
                        animations[animationIndex].keyframes[targetPoseJoint].rotations.push_back(
                            { time, data[i] }
                        );
                    }
                }
            }
            animations[animationIndex].length = maxKeyframeTime;
            animations[animationIndex].name = gltfAnimation.name;
        }

        return animations;
    }


    Pose load_gltf_joints(
        const tinygltf::Model& gltfModel,
        size_t gltfSkinIndex,
        std::unordered_map<int, int>& outNodeJointMapping
    )
    {
        if (gltfSkinIndex >= gltfModel.skins.size())
        {
            Debug::log(
                "@load_gltf_joints "
                "skin index outside bounds. "
                "Model skin count: " + std::to_string(gltfModel.skins.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        const tinygltf::Skin& skin = gltfModel.skins[gltfSkinIndex];
        // NOTE: DANGER! NOT HANDLING THE CASE IF ENCOUNTERING NODE THAT ISN'T JOINT IN THE SKIN!
        int rootJointNodeIndex = skin.joints[0];
        Pose bindPose;
        add_gltf_joint(
            gltfModel,
            bindPose,
            -1, // index to pose struct's parent joint. NOT glTF node index!
            rootJointNodeIndex,
            outNodeJointMapping
        );

        // Load inverse bind matrices
        // TODO: Maybe handle this somewhere else.. getting messy here...
        const tinygltf::Accessor& invBindAccess = gltfModel.accessors[skin.inverseBindMatrices];
        const tinygltf::BufferView& invBindBufView = gltfModel.bufferViews[invBindAccess.bufferView];
        const tinygltf::Buffer& invBindBuf = gltfModel.buffers[invBindBufView.buffer];
        size_t offset = invBindBufView.byteOffset + invBindAccess.byteOffset;
        for (size_t i = 0; i < bindPose.joints.size(); ++i)
        {
            Matrix4f inverseBindMatrix(1.0f);
            memcpy((void*)(&inverseBindMatrix), invBindBuf.data.data() + offset, sizeof(Matrix4f));
            bindPose.joints[i].inverseMatrix = inverseBindMatrix;
            offset += sizeof(float) * 16;
        }
        return bindPose;
    }
}
