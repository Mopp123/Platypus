#include "platypus/graphics/Swapchain.h"
#include "DesktopSwapchain.h"
#include "DesktopContext.h"
#include "DesktopRenderPass.h"
#include "platypus/core/platform/desktop/DesktopWindow.h"
#include "platypus/core/Debug.h"
#include "platypus/core/Application.h"
#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>
#include <map>
#include <vulkan/vulkan_core.h>


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

    static VkExtent2D select_extent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, Window& window)
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

    static std::vector<VkImageView> create_image_views(
        VkDevice device,
        const std::vector<VkImage>& images,
        VkFormat format
    )
    {
        std::vector<VkImageView> imageViews(images.size());
        for (size_t i = 0; i < images.size(); ++i)
        {
            const VkImage& image = images[i];
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult createResult = vkCreateImageView(
                device,
                &createInfo,
                nullptr,
                &imageViews[i]
            );
            if (createResult != VK_SUCCESS)
            {
                const std::string errStr(string_VkResult(createResult));
                Debug::log(
                    "@create_image_views "
                    "Failed to create image view for swapchain at index: " + std::to_string(i) + " "
                    "VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
        return imageViews;
    }


    static std::vector<VkFramebuffer> create_framebuffers(
        VkDevice device,
        const std::vector<VkImageView>& imageViews,
        VkRenderPass renderPass,
        VkExtent2D surfaceExtent
    )
    {
        std::vector<VkFramebuffer> framebuffers(imageViews.size());
        for (size_t i = 0; i < framebuffers.size(); ++i)
        {
            VkImageView attachments[] =
            {
                imageViews[i]/*,
                _depthTexture->getVkImageView()*/
            };

            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass = renderPass;
            createInfo.attachmentCount = 1; //2;
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
        outRenderFinishedSemaphores.resize(maxFramesInFlight);
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
            VkResult renderFinishedSemaphoreResult = vkCreateSemaphore(
                device,
                &semaphoreCreateInfo,
                nullptr,
                &outRenderFinishedSemaphores[i]
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
            if (fenceResult != VK_SUCCESS)
            {
                std::string errStr(string_VkResult(renderFinishedSemaphoreResult));
                Debug::log(
                    "@create_sync_objects "
                    "Failed to create 'inFlightFence'! VkResult: " + errStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        }
    }


    Swapchain::Swapchain(Window& window)
    {
        _pImpl = new SwapchainImpl;
        create(window);
    }

    Swapchain::~Swapchain()
    {
        destroy();
        delete _pImpl;
    }

    void Swapchain::create(Window& window)
    {
        const ContextImpl::SwapchainSupportDetails& swapchainSupportDetails = Context::get_impl()->deviceSwapchainSupportDetails;
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
        createInfo.surface = Context::get_impl()->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = selectedFormat.format;
        createInfo.imageColorSpace = selectedFormat.colorSpace;
        createInfo.imageExtent = selectedExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const ContextImpl::QueueFamilyIndices& deviceQueueFamilyIndices = Context::get_impl()->deviceQueueFamilyIndices;
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

        VkDevice device = Context::get_impl()->device;
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

        Debug::log("___TEST___SWAPCHAIN IMAGE COUNT = " + std::to_string(_imageCount));

        _pImpl->handle = swapchain;
        _pImpl->extent = selectedExtent;
        _pImpl->imageFormat = selectedFormat.format;
        _pImpl->images = createdImages;
        _pImpl->imageViews = create_image_views(device, createdImages, selectedFormat.format);
        _pImpl->maxFramesInFlight = _imageCount - 1;
        Debug::log("___TEST___SWAPCHAIN MAX FRAMES IN FLIGHT = " + std::to_string(_pImpl->maxFramesInFlight));

        _renderPass.create(*this);
        _pImpl->framebuffers = create_framebuffers(
            device,
            _pImpl->imageViews,
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
        // NOTE: Not sure is this the best way to throw the device around...
        VkDevice device = Context::get_impl()->device;

        for (size_t i = 0; i < _pImpl->maxFramesInFlight; ++i)
        {
            vkDestroySemaphore(device, _pImpl->imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, _pImpl->renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, _pImpl->inFlightFences[i], nullptr);

            _pImpl->imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            _pImpl->renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            _pImpl->inFlightFences[i] = VK_NULL_HANDLE;
        }
        _pImpl->imageAvailableSemaphores.clear();
        _pImpl->renderFinishedSemaphores.clear();
        _pImpl->inFlightFences.clear();
        _pImpl->inFlightImages.clear();

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

    void Swapchain::recreate(Window& window)
    {
        destroy();
        create(window);
    }

    SwapchainResult Swapchain::acquireImage()
    {
        VkDevice device = Context::get_impl()->device;
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

        //VkSemaphore signalSemaphores[] = { _pImpl->renderFinishedSemaphores[_currentFrame] };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &_pImpl->renderFinishedSemaphores[_currentFrame];//signalSemaphores;

        VkSwapchainKHR swapchains[] = { _pImpl->handle };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;

        presentInfo.pImageIndices = &_currentImageIndex;

        VkResult presentResult = vkQueuePresentKHR(Context::get_impl()->presentQueue, &presentInfo);
        SwapchainResult retResult = SwapchainResult::ERROR;

        if (presentResult == VK_SUCCESS)
        {
            retResult = SwapchainResult::SUCCESS;
        }
        else if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            retResult = SwapchainResult::RESIZE_REQUIRED;
            Debug::log(
                "@Swapchain::present "
                "Unable to present. Window may have been resized. "
                "Resize handling not yet implemented!",
                Debug::MessageType::PLATYPUS_WARNING
            );
        }
        advanceFrame();
        return retResult;
    }

    void Swapchain::advanceFrame()
    {
        _currentFrame = (_currentFrame + 1) % _pImpl->maxFramesInFlight;
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
