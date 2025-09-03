#include "platypus/graphics/Device.hpp"
#include "platypus/core/Debug.h"


namespace platypus
{
    struct DeviceImpl
    {
    };

    DeviceImpl* Device::s_pImpl = nullptr;
    Window* Device::s_pWindow = nullptr;
    size_t Device::s_minUniformBufferOffsetAlignment = 1;
    CommandPool* Device::s_pCommandPool = nullptr;

    void Device::create(Window* pWindow)
    {
        s_pCommandPool = new CommandPool;
    }

    void Device::destroy()
    {
        delete s_pCommandPool;
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
        return 1;
    }

    CommandPool* Device::get_command_pool()
    {
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
