#pragma once

#include "platypus/core/Window.hpp"
#include "CommandBuffer.h"
#include "Swapchain.h"

namespace platypus
{
    struct DeviceImpl;
    class Device
    {
    private:
        static DeviceImpl* s_pImpl;
        static Window* s_pWindow;
        static size_t s_minUniformBufferOffsetAlignment;

    public:
        static void create(Window* pWindow);
        static void destroy();

        // At the moment all rendering is done in a way that we have a single render pass and a single
        // primary command buffer in which we have recorded secondary command buffers.
        // Eventually we submit the primary command buffer for execution into the device's graphics queue.
        // TODO: Completely separate class for "Device" handling Device specific stuff like this
        static void submit_primary_command_buffer(
            Swapchain& swapchain,
            const CommandBuffer& cmdBuf,
            size_t frame
        );

        // *On vulkan side we need to wait for device operations to finish before freeing resources used
        // by these operations
        static void wait_for_operations();

        // *On vulkan, required to re query swapchain support details to recreate swapchain
        static void handle_window_resize();

        // Required for vulkan's descriptor sets using dynamic offsets of uniform buffers.
        // For OpenGL this should be always 1
        static size_t get_min_uniform_buffer_offset_align();

        static DeviceImpl* get_impl();
    };
}
