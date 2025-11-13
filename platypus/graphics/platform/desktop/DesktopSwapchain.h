#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>


namespace platypus
{
    struct SwapchainImpl
    {
        VkSwapchainKHR handle;
        VkExtent2D extent;
        VkFormat imageFormat;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> inFlightImages;
        size_t maxFramesInFlight;
    };
}
