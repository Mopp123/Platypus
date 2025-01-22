#include "platypus/graphics/RenderCommand.h"
#include "DesktopSwapchain.h"
#include "platypus/graphics/RenderPass.h"
#include "DesktopRenderPass.h"
#include "DesktopCommandBuffer.h"
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
            beginInfo.renderPass = swapchain.getRenderPass().getPImpl()->handle;
            beginInfo.framebuffer = swapchain.getPImpl()->framebuffers[swapchain.getCurrentImageIndex()];

            VkClearValue clearColorValue{};
            clearColorValue.color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };

            // TODO: add another clear value, clearing depthStencil (cannot reside in the same VkClearValue!)
            beginInfo.clearValueCount = 1;
            beginInfo.pClearValues = &clearColorValue;

            beginInfo.renderArea.offset = { 0, 0 };
            Extent2D swapchainExtent = swapchain.getExtent();
            beginInfo.renderArea.extent = { swapchainExtent.width, swapchainExtent.height };

            vkCmdBeginRenderPass(
                primaryCmdBuf.getPImpl()->handle,
                &beginInfo,
                VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
            );
        }

        void end_render_pass(const CommandBuffer& commandBuffer)
        {
            vkCmdEndRenderPass(commandBuffer.getPImpl()->handle);
        }

        void exec_secondary_command_buffers(
            const CommandBuffer& primary,
            const std::vector<CommandBuffer>& secondaries
        )
        {
            std::vector<VkCommandBuffer> secondaryHandles(secondaries.size());
            for (size_t i = 0; i < secondaries.size(); ++i)
                secondaryHandles[i] = secondaries[i].getPImpl()->handle;
            vkCmdExecuteCommands(primary.getPImpl()->handle, (uint32_t)secondaryHandles.size(), secondaryHandles.data());
        }
    }
}
