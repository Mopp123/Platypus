#include "platypus/graphics/Swapchain.h"
#include "DesktopSwapchain.h"
#include "platypus/graphics/Device.hpp"
#include "DesktopDevice.hpp"
#include "DesktopContext.hpp"
#include "platypus/core/platform/desktop/DesktopWindow.hpp"
#include "platypus/core/Debug.h"
#include "platypus/assets/Texture.h"
#include "platypus/assets/platform/desktop/DesktopTexture.h"
#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>


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
        //else if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end())
        //{
        //    Debug::log("Selected VK_PRESENT_MODE_IMMEDIATE_KHR for swapchain");
        //    return VK_PRESENT_MODE_IMMEDIATE_KHR;
        //}
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
        // In order which we prefer the most
        const std::vector<VkFormat> desiredFormats = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT
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


    static void create_color_images(
        ImageFormat format,
        uint32_t width,
        uint32_t height,
        const std::vector<VkImage>& swapchainImages,
        std::vector<Image*>& outImages
    )
    {
        for (size_t i = 0; i < swapchainImages.size(); ++i)
        {
            size_t channels = get_image_format_channel_count(format);
            Image* pImage = new Image(nullptr, width, height, channels, format);
            outImages.push_back(pImage);
        }
    }

    static void create_color_textures(
        VkDevice device,
        ImageFormat format,
        const std::vector<VkImage>& swapchainImages,
        std::vector<Texture*>& outTextures
    )
    {
        for (size_t i = 0; i < swapchainImages.size(); ++i)
        {
            VkImageView imageView = create_image_views(
                device,
                { swapchainImages[i] },
                to_vk_format(format),
                VK_IMAGE_ASPECT_COLOR_BIT,
                1
            )[0];
            Texture* pTexture = new Texture(true);
            pTexture->getImpl()->image = swapchainImages[i];
            pTexture->getImpl()->imageView = imageView;
            outTextures.push_back(pTexture);
        }
    }

    static void create_depth_texture(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkExtent2D surfaceExtent,
        VmaAllocator allocator,
        Image** pOutImage,
        Texture** pOutTexture
    )
    {
        VkFormat format = get_supported_depth_image_format(physicalDevice);

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

        VkImage vkImage = VK_NULL_HANDLE;
        VmaAllocation imageAllocation = VK_NULL_HANDLE;
        VkResult createImageResult = vmaCreateImage(
            allocator,
            &createInfo,
            &allocCreateInfo,
            &vkImage,
            &imageAllocation,
            nullptr
        );
        if (createImageResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createImageResult));
            Debug::log(
                "@create_depth_image "
                "Failed to create depth image for swapchain! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        ImageFormat engineImageFormat = to_engine_format(format);
        size_t channelCount = get_image_format_channel_count(engineImageFormat);
        *pOutImage = new Image(
            nullptr,
            surfaceExtent.width,
            surfaceExtent.height,
            channelCount,
            engineImageFormat
        );

        *pOutTexture = new Texture(true);
        (*pOutTexture)->getImpl()->image = vkImage;
        (*pOutTexture)->getImpl()->vmaAllocation = imageAllocation;
        (*pOutTexture)->getImpl()->imageView = create_image_views(
            device,
            { vkImage },
            format,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            1
        )[0];
    }

    static void create_framebuffers(
        const std::vector<Texture*>& colorTextures,
        Texture* pDepthTexture,
        const RenderPass& renderPass,
        VkExtent2D surfaceExtent,
        std::vector<Framebuffer*>& outFramebuffers
    )
    {
        for (Texture* pColorTexture : colorTextures)
        {
            Framebuffer* pFramebuffer = new Framebuffer(
                renderPass,
                { pColorTexture },
                pDepthTexture,
                surfaceExtent.width,
                surfaceExtent.height
            );
            outFramebuffers.push_back(pFramebuffer);
        }
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


    Swapchain::Swapchain(const Window& window) :
        _renderPass(
            RenderPassType::SCREEN_PASS,
            false,
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE |
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_DEPTH_DISCRETE,
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_COLOR |
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_DEPTH
        )
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
        // NOTE: Not sure should this be handle this way...
        if (_imageCount > 1)
            _pImpl->maxFramesInFlight = _imageCount - 1;
        else
            _pImpl->maxFramesInFlight = 1;

        ImageFormat engineImageFormat = to_engine_format(selectedFormat.format);
        create_color_images(
            engineImageFormat,
            selectedExtent.width,
            selectedExtent.height,
            createdImages,
            _colorImages
        );

        create_color_textures(
            device,
            engineImageFormat,
            createdImages,
            _colorTextures
        );

        create_depth_texture(
            device,
            pDeviceImpl->physicalDevice,
            selectedExtent,
            pDeviceImpl->vmaAllocator,
            &_pDepthImage,
            &_pDepthTexture
        );

        _renderPass.create(_colorImages[0]->getFormat(), _pDepthImage->getFormat());

        create_framebuffers(
            _colorTextures,
            _pDepthTexture,
            _renderPass,
            selectedExtent,
            _framebuffers
        );

        create_sync_objects(
            device,
            _pImpl->imageAvailableSemaphores,
            _pImpl->renderFinishedSemaphores,
            _pImpl->inFlightFences,
            _pImpl->inFlightImages,
            createdImages.size(),
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

        for (Texture* pColorTexture : _colorTextures)
            delete pColorTexture;

        for (Image* pColorImage : _colorImages)
            delete pColorImage;

        delete _pDepthTexture;
        delete _pDepthImage;

        _colorTextures.clear();
        _colorImages.clear();

        for (Framebuffer* pFramebuffer : _framebuffers)
            delete pFramebuffer;

        _framebuffers.clear();

        _renderPass.destroy();

        _previousImageCount = _imageCount;
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
