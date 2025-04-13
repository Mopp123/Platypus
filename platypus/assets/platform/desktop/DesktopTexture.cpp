#include "platypus/assets/Texture.h"
#include "DesktopTexture.h"

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/platform/desktop/DesktopBuffers.h"
#include "platypus/graphics/Context.h"
#include "platypus/graphics/platform/desktop/DesktopContext.h"
#include "platypus/graphics/platform/desktop/DesktopCommandBuffer.h"

#include "platypus/core/Debug.h"
#include <vulkan/vk_enum_string_helper.h>


namespace platypus
{
    static void transition_image_layout(
        const CommandPool& commandPool,
        VkImage imageHandle,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t mipLevelCount
    )
    {
        if (mipLevelCount != 1)
        {
            Debug::log(
                "@transition_image_layout "
                "Invalid mip level count: " + std::to_string(mipLevelCount) + " "
                "Mipmapping not yet implemented so mipLevelCount of 1 is required!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer commandBuffer = commandPool.allocCommandBuffers(
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
    static VkFormat channels_to_vk_format(int channels)
    {
        switch (channels)
        {
            case 1: return VK_FORMAT_R8_SRGB;
            case 2: return VK_FORMAT_R8G8_SRGB;
            case 3: return VK_FORMAT_R8G8B8_SRGB;
            case 4: return VK_FORMAT_R8G8B8A8_SRGB;
            default:
                Debug::log(
                    "@channels_to_vk_format "
                    "Invalid channel count: " + std::to_string(channels) + " "
                    "Supported range is 1-4",
                    Debug::MessageType::PLATYPUS_ERROR
                );
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


    // NOTE: This is an exception that impl has it's own destructor!
    // We wanted to be able to pass TextureSampler just as "pure data" in different places without
    // having to manage it's vulkan implementation's lifetime explicitly so this is required!
    // Also important: Texture will manage eventually the sampler's implementation when all TextureSamplers
    // go out of scope!
    // THIS WILL PROBABLY CAUSE ISSUES AND CONFUSION IN FUTURE!
    TextureSamplerImpl::~TextureSamplerImpl()
    {
        vkDestroySampler(
            Context::get_instance()->getImpl()->device,
            handle,
            nullptr
        );
    }

    TextureSampler::TextureSampler(
        TextureSamplerFilterMode filterMode,
        TextureSamplerAddressMode addressMode,
        uint32_t mipLevelCount,
        uint32_t anisotropicFiltering
    )
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

        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        // TODO: Mipmapping
        if (mipLevelCount > 1)
        {
            Debug::log(
                "@TextureSampler::TextureSampler "
                "Mipmapping not supported yet so you need to provide 1 for this argument!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            //createInfo.minLod = 0.0f; // Optional
            // NOTE: Should rather use: VK_LOD_CLAMP_NONE for maxLod, so we don't need mip level count here!
            // https://www.reddit.com/r/vulkan/comments/1hgq0fz/texture_samplers_best_practice/
            //createInfo.maxLod = (float)mipLevelCount;
            //createInfo.mipLodBias = 0.0f; // Optional
        }
        else
        {
            createInfo.mipLodBias = 0.0f;
            createInfo.minLod = 0.0f;
            createInfo.maxLod = 0.0f;
        }

        VkSampler handle = VK_NULL_HANDLE;
        VkResult createResult = vkCreateSampler(
            Context::get_instance()->getImpl()->device,
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
        _pImpl(other._pImpl)
    {}

    TextureSampler::~TextureSampler()
    {}


    Texture::Texture(
        const CommandPool& commandPool,
        const Image* pImage,
        const TextureSampler& pSampler
    ) :
        Asset(AssetType::ASSET_TYPE_TEXTURE),
        _pSamplerImpl(pSampler.getImpl())
    {
        // NOTE: Not sure if our buffers can be used as staging buffers here without modifying?
        Buffer* pStagingBuffer = new Buffer(
            commandPool,
            (void*)pImage->getData(),
            1, // Single element size is 8 bit "pixel"
            pImage->getSize(),
            BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_SRC_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );

        VkFormat imageFormat = channels_to_vk_format(pImage->getChannels());
        if (imageFormat == VK_FORMAT_R8G8B8_SRGB)
        {
            Debug::log(
                "@Texture::Texture "
                "3 channel textures have limited support on different devices! "
                "You should provide 4 channel textures for wider support!",
                Debug::MessageType::PLATYPUS_ERROR
            );
        }

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = (uint32_t)pImage->getWidth();
        imageCreateInfo.extent.height = (uint32_t)pImage->getHeight();
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1; // TODO: Mipmap support!
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = imageFormat;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        VkImage imageHandle = VK_NULL_HANDLE;
        VmaAllocation vmaAllocation = VK_NULL_HANDLE;
        const ContextImpl* pContextImpl = Context::get_instance()->getImpl();
        VkResult createImageResult = vmaCreateImage(
            pContextImpl->vmaAllocator,
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
            commandPool,
            imageHandle,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1 // TODO: Mipmapping
        );
        copy_buffer_to_image(
            commandPool,
            pStagingBuffer->getImpl()->handle,
            imageHandle,
            (uint32_t)pImage->getWidth(),
            (uint32_t)pImage->getHeight()
        );
        transition_image_layout(
            commandPool,
            imageHandle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1 // TODO: Mipmapping
        );

        delete pStagingBuffer;

        // NOTE: Possible issue here!
        // *We createsingle texture here for multiple descriptor sets.
        // *If want to create texture which is to be updated dynamically
        //  -> The updating probably breaks the system since the same texture may be in use at the time
        //  of the update by the descriptor sets!
        VkImageView imageView = create_image_views(
            Context::get_instance()->getImpl()->device,
            { imageHandle },
            imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
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
            const ContextImpl* pContextImpl = Context::get_instance()->getImpl();
            vkDestroyImageView(pContextImpl->device, _pImpl->imageView, nullptr);
            vmaDestroyImage(pContextImpl->vmaAllocator, _pImpl->image, _pImpl->vmaAllocation);
            delete _pImpl;
        }
    }
}
