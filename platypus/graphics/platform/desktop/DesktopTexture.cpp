#include "platypus/graphics/Texture.h"
#include "DesktopTexture.h"
#include "platypus/graphics/Buffers.h"
#include "DesktopBuffers.h"
#include "platypus/graphics/Context.h"
#include "DesktopContext.h"
#include "DesktopCommandBuffer.h"
#include "platypus/core/Debug.h"
#include <vulkan/vulkan_core.h>
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


    Texture::Texture(const CommandPool& commandPool, const Image* pImage)
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

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = (uint32_t)pImage->getWidth();
        imageCreateInfo.extent.height = (uint32_t)pImage->getHeight();
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1; // TODO: Mipmap support!
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = channels_to_vk_format(pImage->getChannels());
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

        std::vector<VkImageView> imageViews = create_image_views(
            device,
            { *outImage },
            format,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );
        *outImageView = imageViews[0];

    }

    Texture::~Texture()
    {
    }
}
