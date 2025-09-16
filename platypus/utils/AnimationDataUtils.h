#pragma once

#include "platypus/utils/Maths.h"
#include <vector>
#include <string>
#include <cstdint>


namespace platypus
{
    struct Joint
    {
        Vector3f translation;
        Quaternion rotation;
        Vector3f scale;
        Matrix4f matrix; // Created only for the bind pose. Transforms the joint to its correct initial pos, scale, orientation... If I recall correctly...
        Matrix4f inverseMatrix; // Created only for the bind pose
        std::string name; // NOTE: Not sure if this makes stuff slow...
    };

    struct Pose
    {
        std::vector<Joint> joints;
        std::vector<std::vector<uint32_t>> jointChildMapping;
    };

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

    struct JointAnimationData
    {
        std::vector<TranslationKey> translations;
        std::vector<RotationKey> rotations;
    };

    struct KeyframeAnimationData
    {
        float length = 0.0f;
        // Indexing of these follows the bind pose's joints' indexing
        // which this animation is ment for.
        std::vector<JointAnimationData> keyframes;
    };
}
