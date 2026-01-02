#pragma once

#include "platypus/utils/Maths.h"
#include "CommandBuffer.h"
#include "RenderPass.hpp"
#include "Framebuffer.hpp"
#include "Pipeline.h"
#include "Descriptors.h"


namespace platypus
{
    // NOTE: Maybe some better namespace for this...
    namespace render
    {
        // TODO: Make this replace the other begin renderpass funcs!
        // NOTE: If renderPass is offscreen pass
        //  -> desktop impl transitions the depth texture back to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        //  if it was transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL earlier.
        //  This should probably be done by the renderPass implicitly, but just to make sure for now...
        void begin_render_pass(
            CommandBuffer& commandBuffer,
            const RenderPass& renderPass,
            Framebuffer* pFramebuffer,
            const Vector4f& clearColor
        );

        void end_render_pass(
            CommandBuffer& commandBuffer,
            const RenderPass& renderPass
        );

        void exec_secondary_command_buffers(
            const CommandBuffer& primary,
            const std::vector<CommandBuffer>& secondaries
        );

        void bind_pipeline(
            CommandBuffer& commandBuffer,
            const Pipeline& pipeline
        );

        // NOTE: With Vulkan, if dynamic state not set for pipeline this needs to be called
        // BEFORE BINDING THE PIPELINE!
        void set_viewport(
            const CommandBuffer& commandBuffer,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight,
            float viewportMinDepth = 0.0f,
            float viewportMaxDepth = 1.0f
        );

        void set_scissor(
            const CommandBuffer& commandBuffer,
            Rect2D scissor
        );

        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_vertex_buffers(
            const CommandBuffer& commandBuffer,
            const std::vector<const Buffer*>& vertexBuffers
        );

        // NOTE: Not sure should we pass buffers as ptrs here.
        // ->There could probably be a better way
        void bind_index_buffer(
            const CommandBuffer& commandBuffer,
            const Buffer* indexBuffer
        );

        // TODO: Implement!
        void push_constants(
            CommandBuffer& commandBuffer,
            ShaderStageFlagBits shaderStageFlags,
            uint32_t offset,
            uint32_t size,
            const void* pValues,
            std::vector<UniformInfo> glUniformInfo // Only used on opengl side NOTE: Why this passed by value!?
        );

        void bind_descriptor_sets(
            CommandBuffer& commandBuffer,
            const std::vector<DescriptorSet>& descriptorSets,
            const std::vector<uint32_t>& offsets
        );

        void draw_indexed(
            const CommandBuffer& commandBuffer,
            uint32_t count,
            uint32_t instanceCount
        );

        void draw(
            const CommandBuffer& commandBuffer,
            uint32_t count
        );
    }
}
