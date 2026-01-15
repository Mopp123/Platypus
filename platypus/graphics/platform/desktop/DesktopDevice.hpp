#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <unordered_map>


namespace platypus
{
    // *Maybe this should rather be in DesktopWindow?
    struct WindowSurfaceProperties
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    enum QueueFamilyFlagBits
    {
        QUEUE_FAMILY_NONE = 0x0,
        QUEUE_FAMILY_GRAPHICS = 0x1,
        QUEUE_FAMILY_PRESENT = 0x2,
    };

    struct QueueProperties
    {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;
        uint32_t queueFlags = QueueFamilyFlagBits::QUEUE_FAMILY_NONE;
    };

    struct PhysicalDevice
    {
        VkPhysicalDevice handle = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties;
        QueueProperties queueProperties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::unordered_map<VkFormat, VkFormatProperties> supportedFormats;

        WindowSurfaceProperties windowSurfaceProperties;
    };

    struct DeviceImpl
    {
        PhysicalDevice physicalDevice;
        VkDevice device = VK_NULL_HANDLE;

        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VmaAllocator vmaAllocator;
    };

    bool is_format_supported(VkFormat format);
}
