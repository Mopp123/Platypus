#pragma once

#include <cstdint>


namespace platypus
{
    struct FramebufferImpl
    {
        uint32_t id = 0;
        uint32_t depthRenderBuffer = 0;
    };
}
