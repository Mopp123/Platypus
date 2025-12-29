#pragma once

#include "platypus/graphics/Pipeline.h"
#include <vulkan/vulkan.h>


namespace platypus
{
    VkPipelineStageFlags to_vk_pipeline_stage(PipelineStage stage);

    struct PipelineImpl
    {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline handle = VK_NULL_HANDLE;
    };
}
