#include "platypus/graphics/Swapchain.h"
#include "DesktopSwapchain.h"
#include "platypus/graphics/Device.hpp"
#include "DesktopDevice.hpp"
#include "DesktopContext.hpp"
#include "DesktopRenderPass.h"
#include "platypus/core/platform/desktop/DesktopWindow.hpp"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>
#include <map>


namespace platypus
{
    static VkSurfaceFormatKHR select_surface_format(const std::vector<VkSurfaceFormatKHR>& surfaceFormats)
    {
        for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return surfaceFormat;
        }
        return surfaceFormats[0];
    }

    // NOTE: On at least some Linux, VK_PRESENT_MODE_FIFO_KHR on windowed mode stutters and tears for
    // some reason and VK_PRESENT_MODE_IMMEDIATE_KHR seems to work a lot better...
    // -> Some window manager issue?
    static VkPresentModeKHR select_present_mode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end())
        {
            Debug::log("Selected VK_PRESENT_MODE_MAILBOX_KHR for swapchain");
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end())
        {
            Debug::log("Selected VK_PRESENT_MODE_IMMEDIATE_KHR for swapchain");
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        Debug::log("Selected VK_PRESENT_MODE_FIFO_KHR for swapchain");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D select_extent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, const Window& window)
    {
        if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
        {
            return surfaceCapabilities.currentExtent;
        }
        else
        {
            int widthI = 0;
            int heightI = 0;
            window.getSurfaceExtent(&widthI, &heightI);
            VkExtent2D extent = { (uint32_t)widthI, (uint32_t)heightI };
            extent.width = std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
            extent.height = std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
            return extent;
        }
    }

    static VkFormat get_supported_depth_image_format(VkPhysicalDevice physicalDevice)
    {
        std::vector<VkFormat> desiredFormats =
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        VkFormatFeatureFlags featureFlags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        for (VkFormat format : desiredFormats)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
            // NOTE: Atm assuming to use VkImageTiling = VK_IMAGE_TILING_OPTIMAL!
            if ((properties.optimalTilingFeatures & featureFlags) == featureFlags)
            {
                return format;
            }
        }
        Debug::log(
            "@get_supported_depth_image_format "
            "Failed to find suitable depth image format!",
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);
        return desiredFormats[0];
    }

    static void create_depth_texture(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D surfaceExtent,
        VmaAllocator allocator,
        VkFormat* outImageFormat,
        VkImage* outImage,
        VkImageView* outImageView,
        VmaAllocation* outAllocation
    )
    {
        VkFormat format = get_supported_depth_image_format(physicalDevice);
        *outImageFormat = format;

        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.extent.width = surfaceExtent.width;
        createInfo.extent.height = surfaceExtent.height;
        createInfo.extent.depth = 1; // Wasn't sure is depth put to 1 in the param, so thats why...
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;
        createInfo.format = format;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // TODO: Make this modifyable and make sure the used tiling is supported!
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        VkResult createImageResult = vmaCreateImage(
            allocator,
            &createInfo,
            &allocCreateInfo,
            outImage,
            outAllocation,
            nullptr
        );
        if (createImageResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createImageResult));
            Debug::log(
                "@create_depth_textures "
                "Failed to create depth image for swapchain! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        std::vector<VkImageView> imageViews = create_image_views(
            device,
            { *outImage },
            format,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1
        );
        *outImageView = imageViews[0];
    }

    static std::vector<VkFramebuffer> create_framebuffers(
        VkDevice device,
        const std::vector<VkImageView>& imageViews,
        VkImageView depthImageView,
        VkRenderPass renderPass,
        VkExtent2D surfaceExtent
    )
    {
        std::vector<VkFramebuffer> framebuffers(imageViews.size());
        for (size_t i = 0; i < framebuffers.size(); ++i)
        {
            VkImageView attachments[] =
            {
                imageViews[i],
                depthImageView
            };

            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = 2;
            createInfo.pAttachments = attachments;
            createInfo.width = surfaceExtent.width;
            createInfo.height = surfaceExtent.height;
            createInfo.layers = 1;

            VkResult createResult = vkCreateFramebuffer(
                device,
                &createInfo,
                nullptr,
                &framebuffers[i]
            );
            if (createResult != VK_SUCCESS)
            {
                const std::string errStr(string_VkResult(createResult));
                Debug::log(
                    "@create_framebuffers "
                    "Failed to create framebuffers for Swapchain! VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        return framebuffers;
    }


    static void create_sync_objects(
        VkDevice device,
        std::vector<VkSemaphore>& outImageAvailableSemaphores,
        std::vector<VkSemaphore>& outRenderFinishedSemaphores,
        std::vector<VkFence>& outInFlightFences,
        std::vector<VkFence>& outInFlightImages,
        size_t swapchainImageCount,
        size_t maxFramesInFlight
    )
    {
        outImageAvailableSemaphores.resize(maxFramesInFlight);
        outInFlightFences.resize(maxFramesInFlight);
        outInFlightImages.resize(swapchainImageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < maxFramesInFlight; ++i)
        {
            VkResult imageAvailableSemaphoreResult = vkCreateSemaphore(
                device,
                &semaphoreCreateInfo,
                nullptr,
                &outImageAvailableSemaphores[i]
            );
            VkResult fenceResult = vkCreateFence(
                device,
                &fenceCreateInfo,
                nullptr,
                &outInFlightFences[i]
            );
            if (imageAvailableSemaphoreResult != VK_SUCCESS)
            {
                std::string errStr(string_VkResult(imageAvailableSemaphoreResult));
                Debug::log(
                    "@create_sync_objects "
                    "Failed to create 'imageAvailableSemaphore'! VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            if (fenceResult != VK_SUCCESS)
            {
                std::string errStr(string_VkResult(fenceResult));
                Debug::log(
                    "@create_sync_objects "
                    "Failed to create 'inFlightFence'! VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }

        // NOTE: Detected new validation error with newer Vulkan version and Validation layer
        //  -> Previously were using same amount of "render finished semaphores" (semaphores to
        //  wait before presenting) as frames in flight (hardcoded 2).
        //      -> This leads to potentially using same semaphore for multiple swapchain images.
        //          -> Disabling validation errors caused the system to work JUST BY ACCIDENT!
        //  -> NEED TO USE SEPARATE "render finished semaphore" PER SWAPCHAIN IMAGE!!!
        outRenderFinishedSemaphores.resize(swapchainImageCount);
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkResult renderFinishedSemaphoreResult = vkCreateSemaphore(
                device,
                &semaphoreCreateInfo,
                nullptr,
                &outRenderFinishedSemaphores[i]
            );

            if (renderFinishedSemaphoreResult != VK_SUCCESS)
            {
                std::string errStr(string_VkResult(renderFinishedSemaphoreResult));
                Debug::log(
                    "@create_sync_objects "
                    "Failed to create 'renderFinishedSemaphore'! VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
    }


    Swapchain::Swapchain(const Window& window)
    {
        _pImpl = new SwapchainImpl;
        create(window);
    }

    Swapchain::~Swapchain()
    {
        destroy();
        delete _pImpl;
    }

    void Swapchain::create(const Window& window)
    {
        DeviceImpl* pDeviceImpl = Device::get_impl();
        const DeviceImpl::SurfaceDetails& surfaceDetails = pDeviceImpl->surfaceDetails;
        const VkSurfaceCapabilitiesKHR& surfaceCapabilities = surfaceDetails.capabilities;
        VkSurfaceFormatKHR selectedFormat = select_surface_format(surfaceDetails.formats);
        VkPresentModeKHR selectedPresentMode = select_present_mode(surfaceDetails.presentModes);
        VkExtent2D selectedExtent = select_extent(surfaceCapabilities, window);

        // "For maximum efficiency" ...on some lesser systems cause mem to run out?
        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
            imageCount = surfaceCapabilities.maxImageCount;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = window.getImpl()->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = selectedFormat.format;
        createInfo.imageColorSpace = selectedFormat.colorSpace;
        createInfo.imageExtent = selectedExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const DeviceImpl::QueueFamilyIndices& deviceQueueFamilyIndices = pDeviceImpl->queueFamilyIndices;
        uint32_t usedIndices[2] = {
            deviceQueueFamilyIndices.graphicsFamily,
            deviceQueueFamilyIndices.presentFamily
        };

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

        VkDevice device = pDeviceImpl->device;
        VkSwapchainKHR swapchain;
        VkResult createResult = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
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

        vkGetSwapchainImagesKHR(device, swapchain, &_imageCount, nullptr);
        std::vector<VkImage> createdImages(_imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &_imageCount, createdImages.data());

        _pImpl->handle = swapchain;
        _pImpl->extent = selectedExtent;
        _pImpl->imageFormat = selectedFormat.format;
        _pImpl->images = createdImages;
        _pImpl->imageViews = create_image_views(
            device,
            createdImages,
            selectedFormat.format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1
        );
        _pImpl->maxFramesInFlight = _imageCount - 1;

        create_depth_texture(
            device,
            pDeviceImpl->physicalDevice,
            selectedExtent,
            pDeviceImpl->vmaAllocator,
            &_pImpl->depthImageFormat,
            &_pImpl->depthImage,
            &_pImpl->depthImageView,
            &_pImpl->depthImageAllocation
        );

        _renderPass.create(*this);

        _pImpl->framebuffers = create_framebuffers(
            device,
            _pImpl->imageViews,
            _pImpl->depthImageView,
            _renderPass.getImpl()->handle,
            selectedExtent
        );

        create_sync_objects(
            device,
            _pImpl->imageAvailableSemaphores,
            _pImpl->renderFinishedSemaphores,
            _pImpl->inFlightFences,
            _pImpl->inFlightImages,
            _pImpl->images.size(),
            _pImpl->maxFramesInFlight
        );

        Debug::log("Swapchain created");
    }

    void Swapchain::destroy()
    {
        DeviceImpl* pDeviceImpl = Device::get_impl();
        VkDevice device = pDeviceImpl->device;

        for (VkSemaphore semaphore : _pImpl->imageAvailableSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);

        for (VkSemaphore semaphore : _pImpl->renderFinishedSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);

        for (VkFence fence : _pImpl->inFlightFences)
            vkDestroyFence(device, fence, nullptr);

        _pImpl->imageAvailableSemaphores.clear();
        _pImpl->renderFinishedSemaphores.clear();
        _pImpl->inFlightFences.clear();
        _pImpl->inFlightImages.clear();

        vkDestroyImageView(device, _pImpl->depthImageView, nullptr);
        vmaDestroyImage(
            pDeviceImpl->vmaAllocator,
            _pImpl->depthImage,
            _pImpl->depthImageAllocation
        );

        _renderPass.destroy();
        for (VkFramebuffer framebuffer : _pImpl->framebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        _pImpl->framebuffers.clear();

        for (VkImageView imageView : _pImpl->imageViews)
            vkDestroyImageView(device, imageView, nullptr);
        _pImpl->imageViews.clear();

        _imageCount = 0;

        vkDestroySwapchainKHR(device, _pImpl->handle, nullptr);
        _pImpl->handle = VK_NULL_HANDLE;
    }

    void Swapchain::recreate(const Window& window)
    {
        destroy();
        create(window);
    }

    SwapchainResult Swapchain::acquireImage()
    {
        VkDevice device = Device::get_impl()->device;
        vkWaitForFences(
            device,
            1,
            &_pImpl->inFlightFences[_currentFrame],
            VK_TRUE,
            UINT64_MAX
        );

        VkResult result = vkAcquireNextImageKHR(
            device,
            _pImpl->handle,
            UINT64_MAX,
            _pImpl->imageAvailableSemaphores[_currentFrame],
            VK_NULL_HANDLE,
            &_currentImageIndex
        );

        if (result == VK_SUCCESS)
        {
            return SwapchainResult::SUCCESS;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            Debug::log("@Swapchain::acquireImage VK_SUBOPTIMAL_KHR", Debug::MessageType::PLATYPUS_WARNING);
            return SwapchainResult::SUCCESS;
        }
        else if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            Debug::log("@Swapchain::acquireImage VK_ERROR_OUT_OF_DATE_KHR", Debug::MessageType::PLATYPUS_WARNING);
            return SwapchainResult::RESIZE_REQUIRED;
        }
        const std::string errStr(string_VkResult(result));
        Debug::log(
            "@Swapchain::acquireImage "
            "Failed to acquire image! VkResult: " + errStr,
            Debug::MessageType::PLATYPUS_ERROR
        );
        return SwapchainResult::ERROR;
    }

    SwapchainResult Swapchain::present()
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &_pImpl->renderFinishedSemaphores[_currentImageIndex];//signalSemaphores;

        VkSwapchainKHR swapchains[] = { _pImpl->handle };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;

        presentInfo.pImageIndices = &_currentImageIndex;

        VkResult presentResult = vkQueuePresentKHR(
            Device::get_impl()->presentQueue,
            &presentInfo
        );
        SwapchainResult retResult = SwapchainResult::ERROR;

        if (presentResult == VK_SUCCESS)
        {
            retResult = SwapchainResult::SUCCESS;
        }
        else if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            retResult = SwapchainResult::RESIZE_REQUIRED;
        }
        _currentFrame = (_currentFrame + 1) % _pImpl->maxFramesInFlight;
        return retResult;
    }

    size_t Swapchain::getMaxFramesInFlight() const
    {
        return _pImpl->maxFramesInFlight;
    }

    Extent2D Swapchain::getExtent() const
    {
        return { _pImpl->extent.width, _pImpl->extent.height };
    }
}
