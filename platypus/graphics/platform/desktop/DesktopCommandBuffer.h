#pragma once

#include "platypus/graphics/Framebuffer.hpp"
#include <vulkan/vulkan.h>


namespace platypus
{
    struct CommandBufferImpl
    {
        VkCommandBuffer handle = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        Framebuffer* pBoundFramebuffer = nullptr;
    };

    struct CommandPoolImpl
    {
        VkCommandPool handle = VK_NULL_HANDLE;
    };

}
