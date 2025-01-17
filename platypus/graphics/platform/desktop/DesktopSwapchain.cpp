#include "platypus/graphics/Swapchain.h"
#include "DesktopSwapchain.h"
#include "DesktopContext.h"
#include "platypus/core/platform/desktop/DesktopWindow.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
    {
        for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return surfaceFormat;
        }
        return surfaceFormats[0];
    }

    VkPresentModeKHR select_present_mode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        for (const VkPresentModeKHR& mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D select_extent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, Window& window)
    {
        int widthI = 0;
        int heightI = 0;
        window.getSurfaceExtent(&widthI, &heightI);
        VkExtent2D extent = { (uint32_t)widthI, (uint32_t)heightI };
        extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
        return extent;
    }


    Swapchain::Swapchain(Window& window, Context& context)
    {
        _pImpl = new SwapchainImpl;
        create(window, context);
    }

    Swapchain::~Swapchain()
    {
        destroy();
        delete _pImpl;
    }

    void Swapchain::create(Window& window, Context& context)
    {
        const ContextImpl::SwapchainSupportDetails& swapchainSupportDetails = context._pImpl->deviceSwapchainSupportDetails;
        const VkSurfaceCapabilitiesKHR& surfaceCapabilities = swapchainSupportDetails.surfaceCapabilities;
        VkSurfaceFormatKHR selectedFormat = select_surface_format(swapchainSupportDetails.surfaceFormats);
        VkPresentModeKHR selectedPresentMode = select_present_mode(swapchainSupportDetails.presentModes);
        VkExtent2D selectedExtent = select_extent(surfaceCapabilities, window);

        // "For maximum efficiency" ...on some lesser systems cause mem to run out?
        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (imageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            imageCount = surfaceCapabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = context._pImpl->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = selectedFormat.format;
        createInfo.imageColorSpace = selectedFormat.colorSpace;
        createInfo.imageExtent = selectedExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const ContextImpl::QueueFamilyIndices& deviceQueueFamilyIndices = context._pImpl->deviceQueueFamilyIndices;
        uint32_t usedIndices[2] = { deviceQueueFamilyIndices.graphicsFamily, deviceQueueFamilyIndices.presentFamily };

        if (usedIndices[0] == usedIndices[1])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = usedIndices;
        }

        createInfo.preTransform = surfaceCapabilities.currentTransform;

        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = selectedPresentMode;
        createInfo.clipped = VK_TRUE;

        // NOTE: Used when dealing with recreating swapchain due to window resizing, or something else!
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain;
        VkResult createResult = vkCreateSwapchainKHR(context._pImpl->device, &createInfo, nullptr, &swapchain);
        if (createResult != VK_SUCCESS)
        {
            std::string errStr(string_VkResult(createResult));
            Debug::log(
                "@Swapchain::create "
                "Failed to create swapchain! VkResult: " + errStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        uint32_t createdImageCount = 0;
        vkGetSwapchainImagesKHR(context._pImpl->device, swapchain, &createdImageCount, nullptr);
        std::vector<VkImage> createdImages(createdImageCount);
        vkGetSwapchainImagesKHR(context._pImpl->device, swapchain, &createdImageCount, createdImages.data());

        _pImpl->swapchain = swapchain;
        _pImpl->extent = selectedExtent;
        _pImpl->images = createdImages;

        Debug::log("Swapchain created");
    }

    void Swapchain::destroy()
    {
        // NOTE: Not sure is this the best way to throw the device around...
        Context& context = Application::get_instance()->getContext();
        vkDestroySwapchainKHR(context._pImpl->device, _pImpl->swapchain, nullptr);
    }
}
