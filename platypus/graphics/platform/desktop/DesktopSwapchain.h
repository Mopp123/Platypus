#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace platypus
{
    struct SwapchainImpl
    {
        VkSwapchainKHR swapchain;
        VkExtent2D extent;
        std::vector<VkImage> images;
    };
}
