#pragma once

#include "Swapchain.h"
#include "platypus/utils/Maths.h"
#include "CommandBuffer.h"
#include "Pipeline.h"


namespace platypus
{
    // NOTE: Maybe some better namespace for this...
    namespace render
    {
        // To begin using the swapchain's render pass.
        // NOTE: Can only be called for the primary command buffer atm!
        // TODO: Implement Framebuffers and make this take specific framebuffer and render pass instead
        // of the whole swapchain.
        void begin_render_pass(
            const CommandBuffer& primaryCmdBuf,
            const Swapchain& swapchain,
            const Vector4f& clearColor
        );
        void end_render_pass(const CommandBuffer& commandBuffer);

        void exec_secondary_command_buffers(
            const CommandBuffer& primary,
            const std::vector<CommandBuffer>& secondaries
        );

        // TODO: Implement!
        void bind_pipeline(
            const CommandBuffer& commandBuffer,
            const Pipeline& pipeline
        );

        // NOTE: Implement!
        // NOTE: Not sure should this be called before bind_pipeline?
        //  -> earlier I called this before and don't remember the reason. Might be bullshit...
        void set_viewport(
            const CommandBuffer& commandBuffer,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight,
            float viewportMinDepth = 0.0f,
            float viewportMaxDepth = 1.0f
        );

        // NOTE: Implement!
        // NOTE: Not sure is this required for rendering. Put here because of vulkan
        void set_scissor(
            const CommandBuffer& commandBuffer,
            Rect2D scissor
        );

        // NOTE: Implement!
        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_vertex_buffers(
            const CommandBuffer& commandBuffer,
            const std::vector<Buffer*>& vertexBuffers
        );

        // NOTE: Implement!
        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_index_buffer(
            const CommandBuffer& commandBuffer,
            const Buffer* indexBuffer
        );

        void draw_indexed(
            const CommandBuffer& commandBuffer,
            uint32_t count,
            uint32_t instanceCount
        );
    }
}
