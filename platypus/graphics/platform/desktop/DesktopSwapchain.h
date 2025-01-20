#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace platypus
{
    struct SwapchainImpl
    {
        VkSwapchainKHR swapchain;
        VkExtent2D extent;
        VkFormat imageFormat;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
    };
}
