#include "platypus/assets/Texture.hpp"
#include "DesktopTexture.hpp"

#include "platypus/graphics/Buffers.hpp"
#include "platypus/graphics/platform/desktop/DesktopBuffers.hpp"
#include "platypus/graphics/platform/desktop/DesktopPipeline.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/platform/desktop/DesktopDevice.hpp"
#include "platypus/graphics/platform/desktop/DesktopCommandBuffer.hpp"
#include "platypus/graphics/platform/desktop/DesktopContext.hpp"

#include "platypus/core/Debug.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include <cmath>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    // TODO: Use this instead of the old thing (when creating texture using staging buffer)
    void transition_image_layout(
        CommandBuffer& commandBuffer,
        Texture* pTexture,
        ImageLayout newLayout,
        PipelineStage srcStage,
        uint32_t srcAccessMask,
        PipelineStage dstStage,
        uint32_t dstAccessMask,
        uint32_t mipLevelCount
    )
    {
        TextureImpl* pTextureImpl = pTexture->getImpl();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = pTextureImpl->imageLayout;
        VkImageLayout newVkImgLayout = to_vk_image_layout(newLayout);
        barrier.newLayout = newVkImgLayout;
        pTextureImpl->imageLayout = newVkImgLayout;

        // TODO: Make it possible to use for staging image transition (Texture creation)
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = pTextureImpl->image;

        if (is_color_format(pTexture->getImageFormat()))
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevelCount;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        // Wait for...
        VkPipelineStageFlags srcStageMask = to_vk_pipeline_stage(srcStage);
        barrier.srcAccessMask = srcAccessMask;

        // Who'll be using this...
        VkPipelineStageFlags dstStageMask = to_vk_pipeline_stage(dstStage);
        barrier.dstAccessMask = dstAccessMask;

        //VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        //barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        //barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer.getImpl()->handle,
            srcStageMask,
            dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }


    static void transition_image_layout_immediate(
        Texture* pTexture,
        ImageLayout newLayout,
        PipelineStage srcStage,
        uint32_t srcAccessMask,
        PipelineStage dstStage,
        uint32_t dstAccessMask,
        uint32_t mipLevelCount
    )
    {
        CommandBuffer commandBuffer = Device::get_command_pool()->allocCommandBuffers(
            1,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        )[0];
        commandBuffer.beginSingleUse();

        transition_image_layout(
            commandBuffer,
            pTexture,
            newLayout,
            srcStage,
            srcAccessMask,
            dstStage,
            dstAccessMask,
            mipLevelCount
        );
        commandBuffer.finishSingleUse();
    }


    // NOTE: If formats' *_SRGB not supported by the device whole texture creation fails!
    // TODO: Query supported formats and handle depending on requested channels that way
    VkFormat to_vk_format(ImageFormat imageFormat)
    {
        switch (imageFormat)
        {
            // Color formats
            case ImageFormat::R8_SRGB: return VK_FORMAT_R8_SRGB;
            case ImageFormat::R8G8B8_SRGB: return VK_FORMAT_R8G8B8_SRGB;
            case ImageFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;

            case ImageFormat::B8G8R8A8_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
            case ImageFormat::B8G8R8_SRGB: return VK_FORMAT_B8G8R8_SRGB;

            case ImageFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
            case ImageFormat::R8G8B8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
            case ImageFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;

            case ImageFormat::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
            case ImageFormat::B8G8R8_UNORM: return VK_FORMAT_B8G8R8_UNORM;

            // Depth formats
            case ImageFormat::D16_UNORM:          return VK_FORMAT_D16_UNORM;
            case ImageFormat::D32_SFLOAT:         return VK_FORMAT_D32_SFLOAT;
            case ImageFormat::D16_UNORM_S8_UINT:  return VK_FORMAT_D16_UNORM_S8_UINT;
            case ImageFormat::D24_UNORM_S8_UINT:  return VK_FORMAT_D24_UNORM_S8_UINT;
            case ImageFormat::D32_SFLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;

            default:
            {
                PLATYPUS_ASSERT(false);
                return VK_FORMAT_UNDEFINED;
            }
        }
    }

    VkImageLayout to_vk_image_layout(ImageLayout layout)
    {
        switch (layout)
        {
            case ImageLayout::UNDEFINED:            return VK_IMAGE_LAYOUT_UNDEFINED;
            case ImageLayout::TRANSFER_DST_OPTIMAL: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case ImageLayout::SHADER_READ_ONLY_OPTIMAL: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case ImageLayout::COLOR_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case ImageLayout::PRESENT_SRC_KHR: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        PLATYPUS_ASSERT(false);
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    ImageFormat to_engine_format(VkFormat format)
    {
        switch (format)
        {
            // Color formats
            case VK_FORMAT_R8_SRGB:         return ImageFormat::R8_SRGB;
            case VK_FORMAT_R8G8B8_SRGB:     return ImageFormat::R8G8B8_SRGB;
            case VK_FORMAT_R8G8B8A8_SRGB:   return ImageFormat::R8G8B8A8_SRGB;

            case VK_FORMAT_B8G8R8A8_SRGB:   return ImageFormat::B8G8R8A8_SRGB;
            case VK_FORMAT_B8G8R8_SRGB:     return ImageFormat::B8G8R8_SRGB;

            case VK_FORMAT_R8_UNORM:        return ImageFormat::R8_UNORM;
            case VK_FORMAT_R8G8B8_UNORM:    return ImageFormat::R8G8B8_UNORM;
            case VK_FORMAT_R8G8B8A8_UNORM:  return ImageFormat::R8G8B8A8_UNORM;

            case VK_FORMAT_B8G8R8A8_UNORM:  return ImageFormat::B8G8R8A8_UNORM;
            case VK_FORMAT_B8G8R8_UNORM:    return ImageFormat::B8G8R8_UNORM;

            // Depth formats
            case VK_FORMAT_D16_UNORM:          return ImageFormat::D16_UNORM;
            case VK_FORMAT_D32_SFLOAT:         return ImageFormat::D32_SFLOAT;
            case VK_FORMAT_D16_UNORM_S8_UINT:  return ImageFormat::D16_UNORM_S8_UINT;
            case VK_FORMAT_D24_UNORM_S8_UINT:  return ImageFormat::D24_UNORM_S8_UINT;
            case VK_FORMAT_D32_SFLOAT_S8_UINT: return ImageFormat::D32_SFLOAT_S8_UINT;

            default:
            {
                std::string formatStr = string_VkFormat(format);
                Debug::log(
                    "@to_engine_format "
                    "Invalid format: " + formatStr,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return ImageFormat::R8_SRGB;
            }
        }
    }

    static VkFilter to_vk_sampler_filter_mode(TextureSamplerFilterMode mode)
    {
        switch (mode)
        {
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR:  return VK_FILTER_LINEAR;
            case TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR:    return VK_FILTER_NEAREST;
            default: // This should never happen due to arg being enum class
                Debug::log(
                    "@to_vk_sampler_filter_mode "
                    "Invalid TextureSamplerFilterMode!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
        }
        return VK_FILTER_LINEAR;
    }

    static VkSamplerAddressMode to_vk_sampler_address_mode(TextureSamplerAddressMode mode)
    {
        switch (mode)
        {
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT:            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:     return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            default: // This should never happen due to arg being enum class
                Debug::log(
                    "@to_vk_sampler_address_mode "
                    "Invalid TextureSamplerAddressMode!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
        }
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }


    static void generate_mipmaps(
        VkImage imageHandle,
        VkFormat imageFormat,
        int imgWidth,
        int imgHeight,
        uint32_t mipLevelCount,
        VkFilter filterMode
    )
    {
        CommandBuffer commandBuffer = Device::get_command_pool()->allocCommandBuffers(
            1,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        )[0];
        commandBuffer.beginSingleUse();
        VkCommandBuffer cmdBufferHandle = commandBuffer.getImpl()->handle;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = imageHandle;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = imgWidth;
        int32_t mipHeight = imgHeight;

        for (uint32_t i = 1; i < mipLevelCount; ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                cmdBufferHandle,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            int32_t nextMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
            int32_t nextMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
            blit.dstOffsets[1] = { nextMipWidth, nextMipHeight, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                cmdBufferHandle,
                imageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                imageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                filterMode
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(
                cmdBufferHandle,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            if (mipWidth > 1)
                mipWidth /= 2;

            if (mipHeight > 1)
                mipHeight /= 2;
        }

        // Transition also the last mip level to readonlyoptimal
        barrier.subresourceRange.baseMipLevel = mipLevelCount - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBufferHandle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        commandBuffer.finishSingleUse();
    }


    // NOTE: This is an exception that impl has it's own destructor!
    // We wanted to be able to pass TextureSampler just as "pure data" in different places without
    // having to manage it's vulkan implementation's lifetime explicitly so this is required!
    // Also important: Texture will manage eventually the sampler's implementation when all TextureSamplers
    // go out of scope!
    // THIS WILL PROBABLY CAUSE ISSUES AND CONFUSION IN FUTURE!
    TextureSamplerImpl::~TextureSamplerImpl()
    {
        vkDestroySampler(
            Device::get_impl()->device,
            handle,
            nullptr
        );
    }

    TextureSampler::TextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        bool mipmapping,
        uint32_t anisotropicFiltering
    ) :
        _filterMode(filterMode),
        _addressMode(addressMode),
        _mipmapping(mipmapping)
    {
        VkSamplerCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        VkFilter vkFilterMode = to_vk_sampler_filter_mode(filterMode);
        createInfo.magFilter = vkFilterMode;
        createInfo.minFilter = vkFilterMode;
        VkSamplerAddressMode vkSamplerAddressMode = to_vk_sampler_address_mode(addressMode);
        createInfo.addressModeU = vkSamplerAddressMode;
        createInfo.addressModeV = vkSamplerAddressMode;
        createInfo.addressModeW = vkSamplerAddressMode;

        // TODO: Allow anisotropic filtering
        if (anisotropicFiltering > 0)
        {
            Debug::log(
                "@TextureSampler::TextureSampler "
                "Anisotropic filtering not supported yet so you need to provide 0 for this argument!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            // NOTE: Also need to enable device feature + make sure requested anisotropicFiltering is within
            // physical device properties' max limit
            //createInfo.anisotropyEnable = VK_TRUE;
            //createInfo.maxAnisotropy = (float)anisotropicFiltering;
        }
        else
        {
            createInfo.anisotropyEnable = VK_FALSE;
            createInfo.maxAnisotropy = 1.0f;
        }

        createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;

        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        createInfo.mipmapMode = filterMode == TextureSamplerFilterMode::SAMPLER_FILTER_MODE_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        if (mipmapping)
        {
            createInfo.minLod = 0.0f;
            // NOTE: use VK_LOD_CLAMP_NONE for maxLod, so we don't need mip level count here!
            // https://www.reddit.com/r/vulkan/comments/1hgq0fz/texture_samplers_best_practice/
            createInfo.maxLod = VK_LOD_CLAMP_NONE;
            createInfo.mipLodBias = 0.0f;
        }
        else
        {
            createInfo.mipLodBias = 0.0f;
            createInfo.minLod = 0.0f;
            createInfo.maxLod = 0.0f;
        }

        VkSampler handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateSampler(
            Device::get_impl()->device,
            &createInfo,
            nullptr,
            &handle
        );
        if (createResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createResult));
            Debug::log(
                "@TextureSampler::TextureSampler "
                "Failed to create TextureSampler! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        _pImpl = std::make_shared<TextureSamplerImpl>();
        _pImpl->handle = handle;
    }

    TextureSampler::TextureSampler(const TextureSampler& other) :
        _pImpl(other._pImpl),
        _filterMode(other._filterMode),
        _addressMode(other._addressMode),
        _mipmapping(other._mipmapping)
    {}

    TextureSampler::~TextureSampler()
    {}


    Texture::Texture(bool empty) :
        Asset(AssetType::ASSET_TYPE_TEXTURE)
    {
        _pImpl = new TextureImpl;
    }

    Texture::Texture(
        TextureType type,
        const TextureSampler& sampler,
        ImageFormat format,
        uint32_t width,
        uint32_t height
    ):
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _pSamplerImpl(sampler.getImpl()),
        _imageFormat(format)
    {
        VkFormat vkImageFormat = to_vk_format(format);
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = vkImageFormat;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.extent.depth = 1;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (type == TextureType::COLOR_TEXTURE)
            createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        else if (type == TextureType::DEPTH_TEXTURE)
            createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // No idea can VK_IMAGE_USAGE_SAMPLED_BIT be used here!

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        VkImage imageHandle = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        VkResult createImageResult = vmaCreateImage(
            Device::get_impl()->vmaAllocator,
            &createInfo,
            &allocCreateInfo,
            &imageHandle,
            &vmaAllocation,
            nullptr
        );
        if (createImageResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createImageResult));
            Debug::log(
                "@Texture::Texture(FRAMEBUFFER TEST) "
                "Failed to create VkImage for texture! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if (type == TextureType::DEPTH_TEXTURE)
            imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkImageView imageView = create_image_views(
            Device::get_impl()->device,
            { imageHandle },
            vkImageFormat,
            imageAspectFlags,
            1
        )[0];

        _pImpl = new TextureImpl
        {
            imageHandle,
            imageView,
            vmaAllocation
        };
    }

    Texture::Texture(
        const Image* pImage,
        const TextureSampler& sampler,
        uint32_t atlasRowCount
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _pImage(pImage),
        _pSamplerImpl(sampler.getImpl()),
        _atlasRowCount(atlasRowCount)
    {
        ImageFormat imageFormat = _pImage->getFormat();
        if (!is_image_format_valid(imageFormat, pImage->getChannels()))
        {
            Debug::log(
                "@Texture::Texture "
                "Invalid target format: " + image_format_to_string(imageFormat) + " "
                "for image with " + std::to_string(pImage->getChannels()) + " channels",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }
        _imageFormat = imageFormat;

        // NOTE: Not sure if our buffers can be used as staging buffers here without modifying?
        Buffer* pStagingBuffer = new Buffer(
            (void*)pImage->getData(),
            1, // Single element size is 8 bit "pixel"
            pImage->getSize(),
            BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_SRC_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );

        VkFormat vkImageFormat = to_vk_format(imageFormat);

        // Using vkCmdBlit to create mipmaps, so make sure this is supported
        VkFormatProperties imageFormatProperties;
        vkGetPhysicalDeviceFormatProperties(
            Device::get_impl()->physicalDevice,
            vkImageFormat,
            &imageFormatProperties
        );
        if (!(imageFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            Debug::log(
                "@Texture::Texture "
                "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT not supported. "
                "This is currently requierd to generate mipmaps!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        if (vkImageFormat == VK_FORMAT_R8G8B8_SRGB)
        {
            Debug::log(
                "@Texture::Texture "
                "3 channel textures have limited support on different devices! "
                "You should provide 4 channel textures for wider support!",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }

        uint32_t imageWidth = (uint32_t)pImage->getWidth();
        uint32_t imageHeight = (uint32_t)pImage->getHeight();

        VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        uint32_t mipLevelCount = 1;
        if (sampler.isMipmapped())
        {
            mipLevelCount = (uint32_t)(std::floor(
                std::log2(std::max(imageWidth, imageHeight))
            )) + 1;
            imageUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = imageWidth;
        imageCreateInfo.extent.height = imageHeight;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevelCount;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = vkImageFormat;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = imageUsageFlags;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        VkImage imageHandle = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        VkResult createImageResult = vmaCreateImage(
            Device::get_impl()->vmaAllocator,
            &imageCreateInfo,
            &allocCreateInfo,
            &imageHandle,
            &vmaAllocation,
            nullptr
        );
        if (createImageResult != VK_SUCCESS)
        {
            const std::string resultStr(string_VkResult(createImageResult));
            Debug::log(
                "@Texture::Texture "
                "Failed to create VkImage for texture! VkResult: " + resultStr,
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        _pImpl = new TextureImpl;
        _pImpl->image = imageHandle;
        _pImpl->vmaAllocation = vmaAllocation;
        _pImpl->imageLayout = imageCreateInfo.initialLayout;

        transition_image_layout_immediate(
            this, // NOTE: Potential DANGER!
            ImageLayout::TRANSFER_DST_OPTIMAL,
            PipelineStage::TOP_OF_PIPE_BIT,
            0,
            PipelineStage::TRANSFER_BIT,
            MemoryAccessFlagBits::MEMORY_ACCESS_TRANSFER_WRITE_BIT,
            mipLevelCount
        );
        copy_buffer_to_image(
            pStagingBuffer->getImpl()->handle,
            imageHandle,
            imageWidth,
            imageHeight
        );

        if (mipLevelCount > 1)
        {
            PLATYPUS_ASSERT(_pImpl->imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            generate_mipmaps(
                imageHandle,
                vkImageFormat,
                imageWidth,
                imageHeight,
                mipLevelCount,
                to_vk_sampler_filter_mode(sampler.getFilterMode())
            );
        }
        else
        {
            transition_image_layout_immediate(
                this, // NOTE: Potential DANGER!
                ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                PipelineStage::TRANSFER_BIT,
                MemoryAccessFlagBits::MEMORY_ACCESS_TRANSFER_WRITE_BIT,
                PipelineStage::FRAGMENT_SHADER_BIT,
                MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT,
                mipLevelCount
            );
        }

        delete pStagingBuffer;

        // NOTE: Possible issue here!
        // *We createsingle texture here for multiple descriptor sets.
        // *If want to create texture which is to be updated dynamically
        //  -> The updating probably breaks the system since the same texture may be in use at the time
        //  of the update by the descriptor sets!
        VkImageView imageView = create_image_views(
            Device::get_impl()->device,
            { imageHandle },
            vkImageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            mipLevelCount
        )[0];

        _pImpl->imageView = imageView;
    }

    Texture::~Texture()
    {
        if (_pImpl)
        {
            DeviceImpl* pDeviceImpl = Device::get_impl();
            vkDestroyImageView(pDeviceImpl->device, _pImpl->imageView, nullptr);
            if (_pImpl->vmaAllocation != VK_NULL_HANDLE)
                vmaDestroyImage(pDeviceImpl->vmaAllocator, _pImpl->image, _pImpl->vmaAllocation);

            delete _pImpl;
        }
    }
}
