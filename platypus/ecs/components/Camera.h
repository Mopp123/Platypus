#pragma once

#include "platypus/utils/Maths.h"


namespace platypus
{
    struct Camera
    {
        Matrix4f perspectiveProjectionMatrix = Matrix4f(1.0f);
    };
}
