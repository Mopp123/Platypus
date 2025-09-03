#include "Renderer.h"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    Renderer::Renderer(
        const MasterRenderer& masterRenderer,
        DescriptorPool& descriptorPool,
        uint64_t requiredComponentsMask
    ) :
        _masterRendererRef(masterRenderer),
        _descriptorPoolRef(descriptorPool),
        _requiredComponentsMask(requiredComponentsMask)
    {
    }

    Renderer::~Renderer()
    {
    }

    void Renderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = Device::get_command_pool()->allocCommandBuffers(
            count,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void Renderer::freeCommandBuffers()
    {
        for (CommandBuffer& buffer : _commandBuffers)
            buffer.free();
        _commandBuffers.clear();
    }
}
