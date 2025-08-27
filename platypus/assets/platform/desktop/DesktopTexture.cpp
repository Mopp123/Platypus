#include "platypus/assets/Texture.h"
#include "DesktopTexture.h"

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/platform/desktop/DesktopBuffers.h"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/platform/desktop/DesktopDevice.hpp"
#include "platypus/graphics/platform/desktop/DesktopCommandBuffer.h"
#include "platypus/graphics/platform/desktop/DesktopContext.hpp"

#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>
#include <cmath>


namespace platypus
{
    static void transition_image_layout(
        VkImage imageHandle,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t mipLevelCount
    )
    {
        CommandBuffer commandBuffer = Device::get_command_pool()->allocCommandBuffers(
            1,
            CommandBufferLevel::PRIMARY_COMMAND_BUFFER
        )[0];
        commandBuffer.beginSingleUse();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //* We arent transitioning between any queues here...
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = imageHandle;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevelCount;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            Debug::log(
                "@transition_image_layout "
                "Illegal layout transition!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        vkCmdPipelineBarrier(
            commandBuffer.getImpl()->handle,
            sourceStage,
            destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        commandBuffer.finishSingleUse();
    }

    // NOTE: If formats' *_SRGB not supported by the device whole texture creation fails!
    // TODO: Query supported formats and handle depending on requested channels that way
    static VkFormat to_vk_format(ImageFormat imageFormat)
    {
        switch (imageFormat)
        {
            case ImageFormat::R8_SRGB: return VK_FORMAT_R8_SRGB;
            case ImageFormat::R8G8B8_SRGB: return VK_FORMAT_R8G8B8_SRGB;
            case ImageFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;

            case ImageFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
            case ImageFormat::R8G8B8_UNORM: return VK_FORMAT_R8G8B8_UNORM;
            case ImageFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        }
        PLATYPUS_ASSERT(false);
        return VK_FORMAT_UNDEFINED;
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


    Texture::Texture(
        const Image* pImage,
        ImageFormat targetFormat,
        const TextureSampler& sampler,
        uint32_t atlasRowCount
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _pSamplerImpl(sampler.getImpl()),
        _atlasRowCount(atlasRowCount)
    {
        if (!is_image_format_valid(targetFormat, pImage->getChannels()))
        {
            Debug::log(
                "@Texture::Texture "
                "Invalid target format: " + image_format_to_string(targetFormat) + " "
                "for image with " + std::to_string(pImage->getChannels()) + " channels",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }

        // NOTE: Not sure if our buffers can be used as staging buffers here without modifying?
        Buffer* pStagingBuffer = new Buffer(
            (void*)pImage->getData(),
            1, // Single element size is 8 bit "pixel"
            pImage->getSize(),
            BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_SRC_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );

        VkFormat imageFormat = to_vk_format(targetFormat);

        // Using vkCmdBlit to create mipmaps, so make sure this is supported
        VkFormatProperties imageFormatProperties;
        vkGetPhysicalDeviceFormatProperties(
            Device::get_impl()->physicalDevice,
            imageFormat,
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

        if (imageFormat == VK_FORMAT_R8G8B8_SRGB)
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
        imageCreateInfo.format = imageFormat;
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

        transition_image_layout(
            imageHandle,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
            generate_mipmaps(
                imageHandle,
                imageFormat,
                imageWidth,
                imageHeight,
                mipLevelCount,
                to_vk_sampler_filter_mode(sampler.getFilterMode())
            );
        }
        else
        {
            transition_image_layout(
                imageHandle,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1 // TODO: Mipmapping
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
            imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT,
            mipLevelCount
        )[0];

        _pImpl = new TextureImpl
        {
            imageHandle,
            imageView,
            vmaAllocation
        };
    }

    Texture::~Texture()
    {
        if (_pImpl)
        {
            DeviceImpl* pDeviceImpl = Device::get_impl();
            vkDestroyImageView(pDeviceImpl->device, _pImpl->imageView, nullptr);
            vmaDestroyImage(pDeviceImpl->vmaAllocator, _pImpl->image, _pImpl->vmaAllocation);
            delete _pImpl;
        }
    }
}
