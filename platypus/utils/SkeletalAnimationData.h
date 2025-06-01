#pragma once

#include "platypus/utils/Maths.h"
#include <vector>


namespace platypus
{
    struct Joint
    {
        Vector3f translation;
        Quaternion rotation;
        Vector3f scale;
        Matrix4f inverseMatrix; // Created only for the bind pose
    };


    struct Pose
    {
        std::vector<Joint> joints;
        std::vector<std::vector<int>> jointChildMapping;
    };
}
