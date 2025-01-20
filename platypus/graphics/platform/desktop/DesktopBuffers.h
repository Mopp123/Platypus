#pragma once
#include "platypus/graphics/Buffers.h"
#include <vulkan/vulkan.h>


namespace platypus
{
    struct VertexBufferElementImpl
    {
        VkVertexInputAttributeDescription attribDescription{};
    };


    struct VertexBufferLayoutImpl
    {
        VkVertexInputBindingDescription bindingDescription{};
    };


    struct BufferImpl
    {
        VkBuffer handle = VK_NULL_HANDLE;
    };
}
