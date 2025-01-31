#pragma once

#include <vulkan/vulkan.h>


namespace platypus
{
    struct CommandBufferImpl
    {
        VkCommandBuffer handle = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    };

    struct CommandPoolImpl
    {
        VkCommandPool handle = VK_NULL_HANDLE;
    };

}
