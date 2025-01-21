#pragma once

#include <vulkan/vulkan.h>


namespace platypus
{
    struct PipelineImpl
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline handle = VK_NULL_HANDLE;
    };
}
