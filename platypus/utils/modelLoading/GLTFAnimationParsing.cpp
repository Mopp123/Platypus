#include "GLTFAnimationParsing.h"
#include "GLTFFileUtils.h"
#include "platypus/Common.h"
#include "platypus/core/Debug.h"

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


    static Joint create_anim_pose_joint(
        const tinygltf::Model& gltfModel,
        const tinygltf::Animation& gltfAnim,
        const std::vector<tinygltf::AnimationChannel>& channels,
        size_t keyframeIndex
    )
    {
        Joint joint;
        Matrix4f jointTranslationMatrix(1.0f);
        Matrix4f jointRotationMatrix(1.0f);
        Matrix4f jointScaleMatrix(1.0f);
        for (const tinygltf::AnimationChannel& channel : channels)
        {
            const tinygltf::AnimationSampler& sampler = gltfAnim.samplers[channel.sampler];

            // If keyframe count less than asked index -> use last available
            size_t keyframeCount = gltfModel.accessors[sampler.input].count;
            int useKeyframe = keyframeIndex;
            if (keyframeCount <= keyframeIndex)
                useKeyframe = keyframeCount - 1;

            // Get keyframe timestamp
            int keyframeBufferIndex = gltfModel.bufferViews[gltfModel.accessors[sampler.input].bufferView].buffer;
            const tinygltf::Buffer& keyframeBuffer = gltfModel.buffers[keyframeBufferIndex];

            // Get anim data of this channel
            const tinygltf::Accessor& animDataAccessor = gltfModel.accessors[sampler.output];
            Debug::log("___TEST___animDataAccessorCount = " + std::to_string(animDataAccessor.count));
            const tinygltf::BufferView& bufferView = gltfModel.bufferViews[animDataAccessor.bufferView];
            const tinygltf::Buffer& animDataBuffer = gltfModel.buffers[bufferView.buffer];

            // NOTE: Scale anim not implemented!!!
            // TODO: Implement scale anim
            if (channel.target_path == "translation")
            {
                PE_byte* pAnimData = (PE_byte*)&animDataBuffer.data[animDataAccessor.byteOffset + bufferView.byteOffset + (sizeof(Vector3f) * useKeyframe)];
                Vector3f translationValue = *((Vector3f*)pAnimData);

                jointTranslationMatrix[0 + 3 * 4] = translationValue.x;
                jointTranslationMatrix[1 + 3 * 4] = translationValue.y;
                jointTranslationMatrix[2 + 3 * 4] = translationValue.z;

                joint.translation = translationValue;
            }
            else if (channel.target_path == "rotation")
            {
                PE_byte* pAnimData = (PE_byte*)&animDataBuffer.data[animDataAccessor.byteOffset + bufferView.byteOffset + (sizeof(Quaternion) * useKeyframe)];
                Quaternion rotationValue = *((Quaternion*)pAnimData);
                // Found out that with some configuration u can export gltfs from
                // blender without animation sampling -> causing "null rotations" for some keyframes..
                // *propably applies to translations and scaling as well..
                if (rotationValue.length() <= 0.0f)
                {
                    Debug::log(
                        "@create_anim_pose_joint "
                        "Rotation for keyframe: " + std::to_string(useKeyframe) + " "
                        "was null! Check your gltf export settings!",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    //PLATYPUS_ASSERT(false);
                }
                rotationValue = rotationValue.normalize();
                jointRotationMatrix = rotationValue.toRotationMatrix();
                joint.rotation = rotationValue;
            }
        }

        joint.matrix = jointTranslationMatrix * jointRotationMatrix;

        return joint;
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

    struct NodeAnimationData
    {
        Vector3f translation;
        Quaternion rotation;
    };

    static void load_gltf_animations_NEW(tinygltf::Model& gltfModel,
        std::unordered_map<int, std::pair<std::vector<float>, std::vector<Vector3f>>>& nodeTranslations,
        std::unordered_map<int, std::pair<std::vector<float>, std::vector<Quaternion>>>& nodeRotations,
        size_t& outKeyframes
    )
    {
        static const std::set<std::string> allowedInterpolations = { "LINEAR" };
        for (tinygltf::Animation& gltfAnimation : gltfModel.animations)
        {
            for (tinygltf::AnimationChannel& channel : gltfAnimation.channels)
            {
                int targetNode = channel.target_node;
                std::string path = channel.target_path;
                const tinygltf::AnimationSampler& sampler = gltfAnimation.samplers[channel.sampler];
                if (allowedInterpolations.find(sampler.interpolation) == allowedInterpolations.end())
                {
                    std::string allowedStr;
                    int allowedIndex = 0;
                    for (const std::string& s : allowedInterpolations)
                    {
                        allowedStr += s;
                        if (allowedIndex + 1 < allowedInterpolations.size())
                            allowedStr += ", ";

                        ++allowedIndex;
                    }

                    Debug::log(
                        "@load_gltf_animations_NEW "
                        "Animation channel targeting node(index: " + std::to_string(targetNode) + ", name: " + gltfModel.nodes[targetNode].name + ") "
                        "uses " + sampler.interpolation + " interpolation. "
                        "Currently supported interpolations: " + allowedStr + " (spherical linear for rotations).",
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                    return;
                }

                // Get keyframe timestamp
                const tinygltf::Accessor& keyframeAccessor = gltfModel.accessors[sampler.input];
                const tinygltf::BufferView& keyframeBufferView = gltfModel.bufferViews[keyframeAccessor.bufferView];
                const tinygltf::Buffer& keyframeBuffer = gltfModel.buffers[keyframeBufferView.buffer];
                PE_byte* pKeyframeData = (PE_byte*)(keyframeBuffer.data.data()) + keyframeBufferView.byteOffset + keyframeAccessor.byteOffset;
                std::vector<float> keyframes(keyframeAccessor.count);
                memcpy((void*)keyframes.data(), pKeyframeData, sizeof(float) * keyframeAccessor.count);
                outKeyframes = std::max(outKeyframes, keyframes.size());
                //std::string timestamps;
                //for (size_t i = 0; i < keyframes.size(); ++i)
                //{
                //    timestamps += std::to_string(keyframes[i]);
                //    if (i < keyframes.size() - 1)
                //        timestamps += ", ";
                //}
                //Debug::log("___TEST___KEYFRAMES( " + std::to_string(keyframes.size()) + ") : " + timestamps);

                // Get anim data of this channel
                const tinygltf::Accessor& animDataAccessor = gltfModel.accessors[sampler.output];
                //Debug::log("___TEST___Accessor component type = " + gltf_accessor_component_type_to_string(animDataAccessor.componentType) + " " +
                //    "count = " + std::to_string(animDataAccessor.count) + " "
                //    "type = " + gltf_accessor_type_to_string(animDataAccessor.type)
                //);
                const tinygltf::BufferView& bufferView = gltfModel.bufferViews[animDataAccessor.bufferView];
                const tinygltf::Buffer& animDataBuffer = gltfModel.buffers[bufferView.buffer];

                size_t count = animDataAccessor.count;

                PE_byte* pAnimData = (PE_byte*)&animDataBuffer.data[animDataAccessor.byteOffset + bufferView.byteOffset];

                std::string accessorType = gltf_accessor_type_to_string(
                    animDataAccessor.type
                );

                std::string accessorComponentType = gltf_accessor_component_type_to_string(
                    animDataAccessor.componentType
                );

                if (animDataAccessor.sparse.isSparse)
                {
                    Debug::log("___TEST___WAS SPARSE WTF!");
                    PLATYPUS_ASSERT(false);
                }

                // TODO: Scale
                if (path == "translation")
                {
                    // TESTING
                    //Debug::log("___TEST___Translation accessor type = " + accessorType);
                    Debug::log("___TEST___Node: " + std::to_string(targetNode) + " translations:");
                    Debug::log("    accessor count = " + std::to_string(animDataAccessor.count));
                    Debug::log("    keyframes(" + std::to_string(keyframes.size()) + ") = " + float_vector_to_string(keyframes));
                    Debug::log("    INTERPOLATION: " + sampler.interpolation);

                    std::vector<Vector3f> data(animDataAccessor.count);
                    memcpy((void*)data.data(), pAnimData, animDataAccessor.count * sizeof(Vector3f));
                    std::string dataStr = vec3_vector_to_string(data);
                    Debug::log("    data(" + std::to_string(data.size()) + ") = " + dataStr);
                    // ---

                    nodeTranslations[targetNode].first = keyframes;
                    nodeTranslations[targetNode].second = data;
                }
                else if (path == "rotation")
                {
                    // TESTING
                    //Debug::log("___TEST___Rotation accessor type = " + accessorType);
                    Debug::log("___TEST___Rotation accessor component type = " + accessorComponentType);
                    Debug::log("___TEST___Node: " + std::to_string(targetNode) + " rotations:");
                    Debug::log("    accessor count = " + std::to_string(animDataAccessor.count));
                    Debug::log("    keyframes(" + std::to_string(keyframes.size()) + ") = " + float_vector_to_string(keyframes));
                    Debug::log("    INTERPOLATION: " + sampler.interpolation);


                    std::vector<Quaternion> data(animDataAccessor.count);
                    memcpy((void*)data.data(), pAnimData, animDataAccessor.count * sizeof(Quaternion));
                    std::string dataStr = quat_vector_to_string(data);
                    Debug::log("    data(" + std::to_string(data.size()) + ") = " + dataStr);
                    // ---

                    nodeRotations[targetNode].first = keyframes;
                    nodeRotations[targetNode].second = data;
                }
            }
        }
        //PLATYPUS_ASSERT(false);

        //int selectedNode = 0;
        //std::vector<float> translationKeyframes = nodeTranslations[selectedNode].first;
        //std::vector<float> rotationKeyframes = nodeRotations[selectedNode].first;
        //std::vector<Vector3f> translations = nodeTranslations[selectedNode].second;
        //std::vector<Quaternion> rotations = nodeRotations[selectedNode].second;

        //Debug::log("___TEST___Node: " + std::to_string(selectedNode));
        //Debug::log("    translation keyframes(" + std::to_string(translationKeyframes.size()) + ") : " + float_vector_to_string(translationKeyframes));
        //Debug::log("    translations(" + std::to_string(translations.size()) + ") : " + vec3_vector_to_string(translations));
        //Debug::log("    rotation keyframes(" + std::to_string(rotationKeyframes.size()) + ") : " + float_vector_to_string(rotationKeyframes));
        //Debug::log("    rotations(" + std::to_string(rotations.size()) + ") : " + quat_vector_to_string(rotations));
    }

    std::vector<Pose> load_gltf_anim_poses(
        tinygltf::Model& gltfModel,
        const Pose& bindPose,
        std::unordered_map<int, int> nodePoseJointMapping
    )
    {
        // TESTING:
        std::unordered_map<int, std::pair<std::vector<float>, std::vector<Vector3f>>> nodeTranslations;
        std::unordered_map<int, std::pair<std::vector<float>, std::vector<Quaternion>>> nodeRotations;
        size_t keyframeCount = 0;
        load_gltf_animations_NEW(gltfModel, nodeTranslations, nodeRotations, keyframeCount);
        std::vector<Pose> animPoses;
        for (size_t keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
        {
            Pose p;
            p.joints.resize(nodeTranslations.size());
            for (auto it = nodeTranslations.begin(); it != nodeTranslations.end(); ++it)
            {
                int targetJoint = nodePoseJointMapping[it->first];
                Vector3f translation = it->second.second[keyframeIndex];
                p.joints[targetJoint].translation = translation;
            }
            for (auto it = nodeRotations.begin(); it != nodeRotations.end(); ++it)
            {
                PLATYPUS_ASSERT(it->second.second.size() < keyframeCount);
                int targetJoint = nodePoseJointMapping[it->first];
                Quaternion rotation = it->second.second[keyframeIndex];
                PLATYPUS_ASSERT(rotation.length() > 0.0f);

                rotation = rotation.normalize();
                p.joints[targetJoint].rotation = rotation;
            }
            // create joint matrices
            for (Joint& j : p.joints)
            {
                Matrix4f translationMatrix(1.0f);
                translationMatrix[0 + 3 * 4] = j.translation.x;
                translationMatrix[1 + 3 * 4] = j.translation.y;
                translationMatrix[2 + 3 * 4] = j.translation.z;
                Matrix4f rotationMatrix = j.rotation.toRotationMatrix();
                j.matrix = translationMatrix * rotationMatrix;
            }
            animPoses.push_back(p);
        }
        return animPoses;
        // ---

        tinygltf::Animation& gltfAnim = gltfModel.animations[0];

        std::unordered_map<int, std::vector<tinygltf::AnimationChannel>> nodeChannelsMapping;
        // We require all nodes to have same amount of keyframes at same time here!
        size_t maxKeyframes = 0;
        Debug::log("___TEST___anim channel count = " + std::to_string(gltfAnim.channels.size()));
        for (tinygltf::AnimationChannel& channel : gltfAnim.channels)
        {
            const tinygltf::AnimationSampler& sampler = gltfAnim.samplers[channel.sampler];
            size_t keyframeCount = gltfModel.accessors[sampler.input].count;
            maxKeyframes = std::max(maxKeyframes, keyframeCount);
            nodeChannelsMapping[channel.target_node].push_back(channel);
        }

        std::unordered_map<int, std::vector<Joint>> poseJoints;
        for (size_t i = 0; i < maxKeyframes; ++i)
            poseJoints[i].resize(nodePoseJointMapping.size());

        std::unordered_map<int, std::vector<tinygltf::AnimationChannel>>::const_iterator ncIt;
        for (ncIt = nodeChannelsMapping.begin(); ncIt != nodeChannelsMapping.end(); ++ncIt)
        {
            for (size_t keyframeIndex = 0; keyframeIndex < maxKeyframes; ++keyframeIndex)
            {
                int poseJointIndex = nodePoseJointMapping[ncIt->first];
                // gltf appears to provide keyframes in reverse order -> thats why: maxKeyframes - (keyframeIndex + 1)
                poseJoints[maxKeyframes - (keyframeIndex + 1)][poseJointIndex] = create_anim_pose_joint(
                    gltfModel,
                    gltfAnim,
                    ncIt->second,
                    keyframeIndex
                );
            }
        }

        std::vector<Pose> outPoses;
        std::unordered_map<int, std::vector<Joint>>::const_iterator pjIt;
        for (pjIt = poseJoints.begin(); pjIt != poseJoints.end(); ++pjIt)
        {
            Pose pose;
            pose.joints = pjIt->second;
            pose.jointChildMapping = bindPose.jointChildMapping;
            outPoses.push_back(pose);
        }

        return outPoses;
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
