#include "platypus/graphics/RenderCommand.h"
#include "DesktopSwapchain.h"
#include "platypus/graphics/RenderPass.h"
#include "DesktopRenderPass.h"
#include "DesktopCommandBuffer.h"
#include "DesktopPipeline.h"
#include "DesktopBuffers.h"
#include <vulkan/vulkan.h>


namespace platypus
{
    namespace render
    {
        // To begin using the swapchain's render pass.
        // TODO: Implement Framebuffers and make this take specific framebuffer and render pass instead
        // of the whole swapchain.
        void begin_render_pass(
            const CommandBuffer& primaryCmdBuf,
            const Swapchain& swapchain,
            const Vector4f& clearColor
        )
        {
            VkRenderPassBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginInfo.renderPass = swapchain.getRenderPass().getImpl()->handle;
            beginInfo.framebuffer = swapchain.getImpl()->framebuffers[swapchain.getCurrentImageIndex()];

            VkClearValue clearColorValue{};
            clearColorValue.color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a }; // NOTE: Not sure about the ycm syntax error here? might be gaslighting?:D

            // TODO: add another clear value, clearing depthStencil (cannot reside in the same VkClearValue!)
            beginInfo.clearValueCount = 1;
            beginInfo.pClearValues = &clearColorValue;

            beginInfo.renderArea.offset = { 0, 0 };
            Extent2D swapchainExtent = swapchain.getExtent();
            beginInfo.renderArea.extent = { swapchainExtent.width, swapchainExtent.height };

            vkCmdBeginRenderPass(
                primaryCmdBuf.getImpl()->handle,
                &beginInfo,
                VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
            );
        }

        void end_render_pass(const CommandBuffer& commandBuffer)
        {
            vkCmdEndRenderPass(commandBuffer.getImpl()->handle);
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
            const CommandBuffer& commandBuffer,
            const Pipeline& pipeline
        )
        {
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
            viewport.y = viewportY;
            viewport.width = viewportWidth;
            viewport.height = viewportHeight;
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
            const std::vector<Buffer*>& vertexBuffers
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
