#pragma once

#include "platypus/graphics/Buffers.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>


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
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
    };


    void copy_buffer(VkBuffer source, VkBuffer destination, size_t size);

    void copy_buffer_to_image(
        VkBuffer source,
        VkImage destination,
        uint32_t imageWidth,
        uint32_t imageHeight
    );

    VkVertexInputRate to_vk_vertex_input_rate(VertexInputRate inputRate);
    VkFormat to_vk_format_from_shader_datatype(ShaderDataType type);
    VkBufferUsageFlags to_vk_buffer_usage_flags(uint32_t flags);
    VkIndexType to_vk_index_type(size_t bufferElementSize);
}
