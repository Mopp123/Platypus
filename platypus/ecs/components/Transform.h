#pragma once

#include "platypus/utils/Maths.h"


namespace platypus
{
    struct Transform
    {
        Matrix4f localMatrix = Matrix4f(1.0f);
        Matrix4f globalMatrix = Matrix4f(1.0f);
    };
}
