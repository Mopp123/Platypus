#pragma once

#include <vulkan/vulkan.h>
#include "platypus/graphics/Shader.h"


namespace platypus
{
    struct ShaderImpl
    {
        VkShaderModule shaderModule = VK_NULL_HANDLE;
    };

    VkPipelineShaderStageCreateInfo get_pipeline_shader_stage_create_info(
        const Shader* pShader,
        const ShaderImpl * const pImpl
    );

    VkShaderStageFlags to_vk_shader_stage_flags(uint32_t shaderStageFlags);
}
