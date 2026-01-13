#pragma once

#include "platypus/assets/Image.hpp"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>


namespace platypus
{
    VkFormat to_vk_format(ImageFormat format);
    VkImageLayout to_vk_image_layout(ImageLayout layout);
    ImageFormat to_engine_format(VkFormat format);

    struct TextureSamplerImpl
    {
        VkSampler handle = VK_NULL_HANDLE;

        // NOTE: This is an exception that impl has it's own destructor!
        // We wanted to be able to pass TextureSampler just as "pure data" in different places without
        // having to manage it's vulkan implementation's lifetime explicitly so this is required!
        // Also important: Texture will manage eventually the sampler's implementation when all TextureSamplers
        // go out of scope!
        // THIS WILL PROBABLY CAUSE ISSUES AND CONFUSION IN FUTURE!
        ~TextureSamplerImpl();
    };

    struct TextureImpl
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
}
