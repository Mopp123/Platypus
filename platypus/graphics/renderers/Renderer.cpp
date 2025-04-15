#include "Renderer.h"

namespace platypus
{
    Renderer::Renderer(
        const MasterRenderer& masterRenderer,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool,
        uint32_t requiredComponentsMask
    ) :
        _masterRendererRef(masterRenderer),
        _commandPoolRef(commandPool),
        _descriptorPoolRef(descriptorPool),
        _requiredComponentsMask(requiredComponentsMask)
    {
    }

    Renderer::~Renderer()
    {
    }

    void Renderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = _commandPoolRef.allocCommandBuffers(
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

    void Renderer::destroyPipeline()
    {
        _pipeline.destroy();
    }
}
