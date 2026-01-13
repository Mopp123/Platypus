#pragma once

#include "platypus/core/Window.hpp"
#include "CommandBuffer.hpp"
#include "Swapchain.hpp"
#include "platypus/assets/Image.hpp"
#include "platypus/core/Debug.hpp"
#include <algorithm>


namespace platypus
{
    struct DeviceImpl;
    class Device
    {
    private:
        static DeviceImpl* s_pImpl;
        static Window* s_pWindow;
        static size_t s_minUniformBufferOffsetAlignment;
        static CommandPool* s_pCommandPool;
        static std::vector<ImageFormat> s_supportedDepthFormats;
        static std::vector<ImageFormat> s_supportedColorFormats;

    public:
        static void create(Window* pWindow);
        static void destroy();

        // At the moment all rendering is done in a way that we have a single
        // primary command buffer in which we have recorded secondary command buffers.
        // Eventually we submit the primary command buffer for execution into the device's
        // graphics queue.
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

        // Required for descriptor sets using dynamic offsets of uniform buffers.
        static size_t get_min_uniform_buffer_offset_align();

        static CommandPool* get_command_pool();

        static bool is_depth_format_supported(ImageFormat format)
        {
            return std::find_if(
                s_supportedDepthFormats.begin(),
                s_supportedDepthFormats.end(),
                [format](const ImageFormat& supportedFormat)
                {
                    return supportedFormat == format;
                }
            ) != s_supportedDepthFormats.end();
        }

        static bool is_color_format_supported(ImageFormat format)
        {
            return std::find_if(
                s_supportedColorFormats.begin(),
                s_supportedColorFormats.end(),
                [format](const ImageFormat& supportedFormat)
                {
                    return supportedFormat == format;
                }
            ) != s_supportedColorFormats.end();
        }

        static ImageFormat get_first_supported_depth_format()
        {
            if (s_supportedDepthFormats.empty())
            {
                Debug::log(
                    "@Device::get_first_supported_depth_format "
                    "No supported depth formats exist!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return ImageFormat::NONE;
            }
            return s_supportedDepthFormats[0];
        }

        static DeviceImpl* get_impl();
    };
}
