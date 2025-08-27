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
        Matrix4f matrix; // NOTE: Don't remember was this actually used for anything anymore...
        Matrix4f inverseMatrix; // Created only for the bind pose
        std::string name; // NOTE: Not sure if this makes stuff slow...
    };


    struct Pose
    {
        std::vector<Joint> joints;
        std::vector<std::vector<uint32_t>> jointChildMapping;
    };
}
