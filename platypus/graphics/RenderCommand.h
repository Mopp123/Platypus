#pragma once

#include "CommandBuffer.h"
#include "Swapchain.h"
#include "platypus/utils/Maths.h"


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
        void end_render_pass(const CommandBuffer& cmdBuf);
    }
}
