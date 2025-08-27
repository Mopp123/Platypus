#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>


namespace platypus
{

    struct DeviceImpl
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

        struct SurfaceDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

        QueueFamilyIndices queueFamilyIndices;
        VkQueue graphicsQueue;
        VkQueue presentQueue;

        SurfaceDetails surfaceDetails;

        VmaAllocator vmaAllocator;
    };
}
