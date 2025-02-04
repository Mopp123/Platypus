#pragma once

#include <cstdint>

#define PLATYPUS_ENGINE_NAME "Platypus"

// PE stands for Platypus Engine
#define PE_byte char


namespace platypus
{
    struct Extent2D
    {
        uint32_t width = 0;
        uint32_t height = 0;
    };
}
