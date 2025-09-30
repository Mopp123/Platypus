#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace platypus
{
    struct FramebufferImpl
    {
        VkFramebuffer handle = VK_NULL_HANDLE;
    };
}
