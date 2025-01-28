#pragma once
#include "platypus/graphics/Buffers.h"
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


    VkVertexInputRate to_vk_vertex_input_rate(VertexInputRate inputRate);
    VkFormat to_vk_format_from_shader_datatype(ShaderDataType type);
    // NOTE: This is ment to convert a SINGLE flag into a SINGLE VkBufferUsageFlag
    VkBufferUsageFlags to_vk_buffer_usage_flags(uint32_t flags);
    size_t get_shader_datatype_size(ShaderDataType type);
    uint32_t get_shader_datatype_component_count(ShaderDataType type);
    VkIndexType to_vk_index_type(size_t bufferElementSize);
}
