#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace platypus
{
    struct ContextImpl
    {
        VkInstance instance;
        #ifdef PLATYPUS_DEBUG
            VkDebugUtilsMessengerEXT debugMessenger;
        #endif
    };

    std::vector<VkImageView> create_image_views(
        VkDevice device,
        const std::vector<VkImage>& images,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipLevelCount
    );
}
