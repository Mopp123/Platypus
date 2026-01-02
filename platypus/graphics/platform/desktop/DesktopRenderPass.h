#pragma once

#include <vulkan/vulkan.h>


namespace platypus
{
    struct RenderPassImpl
    {
        VkRenderPass handle = VK_NULL_HANDLE;
        VkImageLayout initialColorImageLayout;
        VkImageLayout initialDepthImageLayout;
        VkImageLayout finalColorImageLayout;
        VkImageLayout finalDepthImageLayout;
    };
}
