#pragma once

#include <vulkan/vulkan.h>


namespace platypus
{
    struct ShaderImpl
    {
        VkShaderModule shaderModule = VK_NULL_HANDLE;
    };
}
