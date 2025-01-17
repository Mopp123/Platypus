#pragma once

#include <vulkan/vulkan.h>
#include <vector>


namespace platypus
{
    struct ContextImpl
    {
        enum QueueFamilyFlagBits
        {
            QUEUE_FAMILY_NONE = 0x0,
            QUEUE_FAMILY_GRAPHICS = 0x1,
            QUEUE_FAMILY_PRESENT = 0x2,
        };

        struct QueueFamilyIndices
        {
            uint32_t graphicsFamily;
            uint32_t presentFamily;
            uint32_t queueFlags = QueueFamilyFlagBits::QUEUE_FAMILY_NONE;
        };

        struct SwapchainSupportDetails
        {
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            std::vector<VkSurfaceFormatKHR> surfaceFormats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        VkInstance instance;
        #ifdef PLATYPUS_DEBUG
            VkDebugUtilsMessengerEXT debugMessenger;
        #endif
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        QueueFamilyIndices deviceQueueFamilyIndices;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        SwapchainSupportDetails deviceSwapchainSupportDetails;
    };

}
