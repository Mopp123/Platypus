#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.hpp"
#include <GL/glew.h>


namespace platypus
{
    struct DeviceImpl
    {
    };

    DeviceImpl* Device::s_pImpl = nullptr;
    Window* Device::s_pWindow = nullptr;
    DescriptorPool* Device::s_pDescriptorPool = nullptr;
    size_t Device::s_minUniformBufferOffsetAlignment = 0;
    CommandPool* Device::s_pCommandPool = nullptr;

    // NOTE: Not sure are these really all supported...
    std::vector<ImageFormat> Device::s_supportedDepthFormats = {
        ImageFormat::D16_UNORM,
        ImageFormat::D32_SFLOAT
    };
    std::vector<ImageFormat> Device::s_supportedColorFormats = {
        ImageFormat::R8_SRGB,
        ImageFormat::R8G8B8_SRGB,
        ImageFormat::R8G8B8A8_SRGB,
        ImageFormat::B8G8R8A8_SRGB,
        ImageFormat::B8G8R8_SRGB,
        ImageFormat::R8_UNORM,
        ImageFormat::R8G8B8_UNORM,
        ImageFormat::R8G8B8A8_UNORM,
        ImageFormat::B8G8R8A8_UNORM,
        ImageFormat::B8G8R8_UNORM
    };

    void Device::create(Window* pWindow, size_t maxDescriptorSets)
    {
        s_pCommandPool = new CommandPool;
        int32_t maxUniformBlockSize = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
        int32_t maxUniformBlockBindingPoints = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBlockBindingPoints);
        Debug::log("___TEST___MAX UNIFORM BLOCK SIZE = " + std::to_string(maxUniformBlockSize));
        Debug::log("___TEST___MAX UNIFORM BLOCK BINDINGS = " + std::to_string(maxUniformBlockBindingPoints));

        // NOTE: Not sure what the type of this should be
        GLsizei uboOffsetAlignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboOffsetAlignment);
        s_minUniformBufferOffsetAlignment = (size_t)uboOffsetAlignment;

        s_pDescriptorPool = new DescriptorPool(maxDescriptorSets);
    }

    void Device::destroy()
    {
        delete s_pCommandPool;
        delete s_pDescriptorPool;
    }

    void Device::submit_primary_command_buffer(
        Swapchain& swapchain,
        const CommandBuffer& cmdBuf,
        size_t frame
    )
    {
    }

    void Device::wait_for_operations()
    {
    }

    void Device::handle_window_resize()
    {
    }

    size_t Device::get_min_uniform_buffer_offset_align()
    {
        return s_minUniformBufferOffsetAlignment;
    }

    DescriptorPool* Device::get_descriptor_pool()
    {
        PLATYPUS_ASSERT(s_pDescriptorPool);
        return s_pDescriptorPool;
    }

    CommandPool* Device::get_command_pool()
    {
        PLATYPUS_ASSERT(s_pCommandPool);
        return s_pCommandPool;
    }

    DeviceImpl* Device::get_impl()
    {
        Debug::log(
            "@Device::get_impl Web implementation's device should never be accessed!",
            Debug::MessageType::PLATYPUS_ERROR
        );
        PLATYPUS_ASSERT(false);

        return nullptr;
    }
}
