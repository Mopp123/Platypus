#pragma once

#include "platypus/core/Window.hpp"

// Don't care to mess with adapting to different systems having
// different max push constants sizes, so we'll cap it to max
// quaranteed minimum
#define PLATYPUS_MAX_PUSH_CONSTANTS_SIZE 128

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

        static Window* s_pWindow;
        static ContextImpl* s_pImpl;

        static size_t s_minUniformBufferOffsetAlignment;

    public:
        static void create(const char* appName, Window* pWindow);
        static void destroy();

        // At the moment all rendering is done in a way that we have a single render pass and a single
        // primary command buffer in which we have recorded secondary command buffers.
        // Eventually we submit the primary command buffer for execution into the device's graphics queue.
        // TODO: Completely separate class for "Device" handling Device specific stuff like this
        static void submitPrimaryCommandBuffer(Swapchain& swapchain, const CommandBuffer& cmdBuf, size_t frame);

        // *On vulkan side we need to wait for device operations to finish before freeing resources used
        // by these operations
        static void waitForOperations();

        // *On vulkan, required to re query swapchain support details to recreate swapchain
        static void handleWindowResize();

        // Required for vulkan's descriptor sets using dynamic offsets of uniform buffers.
        // For OpenGL this should be always 1
        static size_t get_min_uniform_buffer_offset_align();
        static ContextImpl* get_impl();
    };
}
