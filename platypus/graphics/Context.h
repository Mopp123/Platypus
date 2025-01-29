#pragma once

#include "platypus/core/Window.h"


namespace platypus
{
    struct ContextImpl;
    class Swapchain;
    class Shader;
    class Buffer;
    class CommandBuffer;

    class Context
    {
    private:
        friend class Swapchain;
        friend class Shader;
        friend class Buffer;
        static ContextImpl* s_pImpl;

    public:
        Context(const char* appName, Window* pWindow);
        ~Context();

        // At the moment all rendering is done in a way that we have a single render pass and a single
        // primary command buffer in which we have recorded secondary command buffers.
        // Eventually we submit the primary command buffer for execution into the device's graphics queue.
        // TODO: Completely separate class for "Device" handling Device specific stuff like this
        void submitPrimaryCommandBuffer(Swapchain& swapchain, const CommandBuffer& cmdBuf, size_t frame);

        // *On vulkan side we need to wait for device operations to finish before freeing resources used
        // by these operations
        void waitForOperations();

        // *On vulkan, required to re query swapchain support details to recreate swapchain
        void handleWindowResize();

        static const ContextImpl * const get_pimpl();
    };
}
