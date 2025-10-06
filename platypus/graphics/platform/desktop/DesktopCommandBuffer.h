#pragma once

#include "platypus/assets/Texture.h"
#include <vulkan/vulkan.h>


namespace platypus
{
    struct CommandBufferImpl
    {
        VkCommandBuffer handle = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        // If this is provided, it's assumed this will be eventually used as a texture in another render pass
        // so the image layout transition is handled for this.
        Texture* pDepthAttachment = nullptr;
    };

    struct CommandPoolImpl
    {
        VkCommandPool handle = VK_NULL_HANDLE;
    };

}
