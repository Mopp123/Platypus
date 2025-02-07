#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>


namespace platypus
{
    struct TextureImpl
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
    };
}
