#include "platypus/graphics/RenderCommand.h"
#include "DesktopSwapchain.h"
#include "DesktopRenderPass.h"
#include "DesktopFramebuffer.hpp"
#include "DesktopCommandBuffer.h"
#include "DesktopPipeline.h"
#include "DesktopBuffers.h"
#include "DesktopShader.h"
#include "DesktopDescriptors.h"
#include "platypus/assets/Texture.h"
#include "platypus/assets/platform/desktop/DesktopTexture.h"
#include "platypus/core/Debug.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>


namespace platypus
{
    namespace render
    {
        void begin_render_pass(
            CommandBuffer& commandBuffer,
            const RenderPass& renderPass,
            const Framebuffer* pFramebuffer,
            Texture* pDepthAttachment,
            const Vector4f& clearColor,
            bool clearDepthBuffer
        )
        {
            if (!pFramebuffer)
            {
                Debug::log(
                    "@begin_render_pass "
                    "Framebuffer required for beginning render pass!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            CommandBufferImpl* pCmdBufferImpl = commandBuffer.getImpl();
            if (renderPass.isOffscreenPass() && pDepthAttachment)
            {
                TextureImpl* pDepthTextureImpl = pDepthAttachment->getImpl();
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = pDepthTextureImpl->imageLayout;
                barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = pDepthTextureImpl->image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                vkCmdPipelineBarrier(
                    pCmdBufferImpl->handle,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            }

            VkRenderPassBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = renderPass.getImpl()->handle;
            CommandBufferImpl* pCommandBufferImpl = commandBuffer.getImpl();
            pCommandBufferImpl->pDepthAttachment = pDepthAttachment;
            beginInfo.framebuffer = pFramebuffer->getImpl()->handle;

            VkClearValue clearColorValue{};
            clearColorValue.color = {{ clearColor.r, clearColor.g, clearColor.b, clearColor.a }};
            VkClearValue clearDepthStencilValue{};
            clearDepthStencilValue.depthStencil = { 1.0f, 0 };
            VkClearValue clearValues[2] = { clearColorValue, clearDepthStencilValue };

            if (clearDepthBuffer)
                beginInfo.clearValueCount = 2;
            else
                beginInfo.clearValueCount = 1;

            beginInfo.pClearValues = clearValues;

            beginInfo.renderArea.offset = { 0, 0 };
            beginInfo.renderArea.extent = { pFramebuffer->getWidth(), pFramebuffer->getHeight() };

            vkCmdBeginRenderPass(
                pCommandBufferImpl->handle,
                &beginInfo,
                VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
            );
        }

        void end_render_pass(CommandBuffer& commandBuffer)
        {
            CommandBufferImpl* pCmdBufferImpl = commandBuffer.getImpl();
            vkCmdEndRenderPass(commandBuffer.getImpl()->handle);

            if (pCmdBufferImpl->pDepthAttachment)
            {
                TextureImpl* pDepthTextureImpl = pCmdBufferImpl->pDepthAttachment->getImpl();

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = pDepthTextureImpl->image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                // (if stencil is present, you might include the stencil bit if you need to preserve it)
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    pCmdBufferImpl->handle,
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
                pDepthTextureImpl->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }

            commandBuffer.getImpl()->pDepthAttachment = nullptr;
        }

        void exec_secondary_command_buffers(
            const CommandBuffer& primary,
            const std::vector<CommandBuffer>& secondaries
        )
        {
            std::vector<VkCommandBuffer> secondaryHandles(secondaries.size());
            for (size_t i = 0; i < secondaries.size(); ++i)
                secondaryHandles[i] = secondaries[i].getImpl()->handle;
            vkCmdExecuteCommands(
                primary.getImpl()->handle,
                (uint32_t)secondaryHandles.size(),
                secondaryHandles.data()
            );
        }

        void bind_pipeline(
            CommandBuffer& commandBuffer,
            const Pipeline& pipeline
        )
        {
            CommandBufferImpl* pCommandBufferImpl = commandBuffer.getImpl();

            // NOTE: Commented out since switching pipeline inside the same Command Buffer
            // should be allowed!
            //  -> Originally this was done to have gui img and font rendering happen using
            //  the same CommandBuffer
            /*
            #ifdef PLATYPUS_DEBUG
            if (commandBuffer.getImpl()->pipelineLayout != VK_NULL_HANDLE)
            {
                Debug::log(
                    "@bind_pipeline "
                    "Command buffer's pipeline layout was already assigned. "
                    "Make sure to end the command buffer at some point after beginning it!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            */
            pCommandBufferImpl->pipelineLayout = pipeline.getImpl()->layout;
            vkCmdBindPipeline(
                commandBuffer.getImpl()->handle,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.getImpl()->handle
            );
        }

        // NOTE: With Vulkan, if dynamic state not set for pipeline this needs to be called
        // BEFORE BINDING THE PIPELINE!
        void set_viewport(
            const CommandBuffer& commandBuffer,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight,
            float viewportMinDepth,
            float viewportMaxDepth
        )
        {
            VkViewport viewport{};
            viewport.x = viewportX;
            //viewport.y = viewportY;
            // NOTE: Not sure if works when changing viewportY, not tested
            viewport.y = viewportHeight - viewportY;
            viewport.width = viewportWidth;
            //viewport.height = viewportHeight;
            viewport.height = -viewportHeight;
            viewport.minDepth = viewportMinDepth;
            viewport.maxDepth = viewportMaxDepth;
            vkCmdSetViewport(commandBuffer.getImpl()->handle, 0, 1, &viewport);
        }

        void set_scissor(
            const CommandBuffer& commandBuffer,
            Rect2D scissor
        )
        {
            VkRect2D vkScissor{};
            vkScissor.offset = { scissor.offsetX, scissor.offsetY };
            vkScissor.extent = { scissor.width, scissor.height };
            vkCmdSetScissor(commandBuffer.getImpl()->handle, 0, 1, &vkScissor);
        }

        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_vertex_buffers(
            const CommandBuffer& commandBuffer,
            const std::vector<const Buffer*>& vertexBuffers
        )
        {
            // NOTE: A little inefficient but necessary
            // TODO: Maybe gather the vk handles differently in the future
            std::vector<VkBuffer> bufferHandles(vertexBuffers.size());
            std::vector<VkDeviceSize> offsets(vertexBuffers.size());
            for (size_t i = 0; i < vertexBuffers.size(); ++i)
            {
                bufferHandles[i] = vertexBuffers[i]->getImpl()->handle;
                offsets[i] = 0;
            }

            vkCmdBindVertexBuffers(
                commandBuffer.getImpl()->handle,
                0,
                (uint32_t)vertexBuffers.size(),
                bufferHandles.data(),
                offsets.data()
            );
        }

        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_index_buffer(
            const CommandBuffer& commandBuffer,
            const Buffer* indexBuffer
        )
        {
            vkCmdBindIndexBuffer(
                commandBuffer.getImpl()->handle,
                indexBuffer->getImpl()->handle,
                0,
                to_vk_index_type(indexBuffer->getDataElemSize())
            );
        }

        // TODO: Implement!
        void push_constants(
            CommandBuffer& commandBuffer,
            ShaderStageFlagBits shaderStageFlags,
            uint32_t offset,
            uint32_t size,
            const void* pValues,
            std::vector<UniformInfo> glUniformInfo // Only used on opengl side
        )
        {
            #ifdef PLATYPUS_DEBUG
            if (commandBuffer.getImpl()->pipelineLayout == VK_NULL_HANDLE)
            {
                Debug::log(
                    "@push_constants "
                    "command buffer's pipeline layout wasn't assigned! "
                    "Make sure you have called bind_pipeline(...) before calling this!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            vkCmdPushConstants(
                commandBuffer.getImpl()->handle,
                commandBuffer.getImpl()->pipelineLayout,
                to_vk_shader_stage_flags(shaderStageFlags),
                offset,
                size,
                pValues
            );
        }

        // NOTE: If using dynamic uniform buffers make sure you have
        // given the correct range when creating the descriptor set!
        void bind_descriptor_sets(
            CommandBuffer& commandBuffer,
            const std::vector<DescriptorSet>& descriptorSets,
            const std::vector<uint32_t>& offsets
        )
        {
            #ifdef PLATYPUS_DEBUG
            if (commandBuffer.getImpl()->pipelineLayout == VK_NULL_HANDLE)
            {
                Debug::log(
                    "@bind_descriptor_sets "
                    "command buffer's pipeline layout wasn't assigned! "
                    "Make sure you have called bind_pipeline(...) before calling this!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif
            const uint32_t descriptorSetCount = (uint32_t)descriptorSets.size();
            std::vector<VkDescriptorSet> handles(descriptorSetCount);
            for (size_t i = 0; i < descriptorSets.size(); ++i)
                handles[i] = descriptorSets[i].getImpl()->handle;

            vkCmdBindDescriptorSets(
                commandBuffer.getImpl()->handle,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                commandBuffer.getImpl()->pipelineLayout,
                0,
                descriptorSetCount,
                handles.data(),
                (uint32_t)offsets.size(),
                offsets.data()
            );
        }

        void draw_indexed(
            const CommandBuffer& commandBuffer,
            uint32_t count,
            uint32_t instanceCount
        )
        {
            vkCmdDrawIndexed(
                commandBuffer.getImpl()->handle,
                count,
                instanceCount,
                0,
                0,
                0
            );
        }
    }
}
