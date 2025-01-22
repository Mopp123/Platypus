#include "MasterRenderer.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"


namespace platypus
{
    MasterRenderer::MasterRenderer(CommandPool& commandPool) :
        _commandPoolRef(commandPool),
        _testRenderer(commandPool)
    {
    }

    MasterRenderer::~MasterRenderer()
    {
    }

    void MasterRenderer::createPipelines(const Swapchain& swapchain)
    {
        const Extent2D swapchainExtent = swapchain.getExtent();
        _testRenderer.createPipeline(swapchain.getRenderPass(), swapchainExtent.width, swapchainExtent.height);
    }

    void MasterRenderer::allocCommandBuffers(uint32_t count)
    {
        _primaryCommandBuffers = _commandPoolRef.allocCommandBuffers(count, CommandBufferLevel::PRIMARY_COMMAND_BUFFER);
        _testRenderer.allocCommandBuffers(count);
    }

    void MasterRenderer::freeCommandBuffers()
    {
        _testRenderer.freeCommandBuffers();

        for (CommandBuffer& buffer : _primaryCommandBuffers)
            buffer.free();
        _primaryCommandBuffers.clear();
    }

    const CommandBuffer& MasterRenderer::recordCommandBuffer(const Swapchain& swapchain, size_t frame)
    {
        if (frame >= _primaryCommandBuffers.size())
        {
            Debug::log(
                "@MasterRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(frame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_primaryCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _primaryCommandBuffers[frame];

        currentCommandBuffer.begin(swapchain.getRenderPass());
        render::begin_render_pass(currentCommandBuffer, swapchain, { 0, 0, 1, 1 });

        // NOTE: We create new copies of secondary command buffers here!
        // Fucking stupid, since the actual command buffers we are using/refering to lives inside the
        // renderer producing the secondary command buffer.
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;

        const Extent2D swapchainExtent = swapchain.getExtent();
        secondaryCommandBuffers.push_back(
                _testRenderer.recordCommandBuffer(swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                frame
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        return currentCommandBuffer;
    }
}
