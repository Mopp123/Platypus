#pragma once

#include <cstdint>

#define PLATYPUS_ENGINE_NAME "Platypus"

namespace platypus
{
    struct Extent2D
    {
        uint32_t width = 0;
        uint32_t height = 0;
    };
}
