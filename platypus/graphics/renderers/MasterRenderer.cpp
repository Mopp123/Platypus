#include "MasterRenderer.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/utils/Maths.h"


namespace platypus
{
    MasterRenderer::MasterRenderer(
        const Swapchain& swapchain,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool
    ) :
        _commandPoolRef(commandPool),
        _testRenderer(swapchain, commandPool, descriptorPool)
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

    void MasterRenderer::destroyPipelines()
    {
        _testRenderer.destroyPipeline();
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

    void MasterRenderer::cleanUp()
    {
        destroyPipelines();
        freeCommandBuffers();
    }

    void MasterRenderer::handleWindowResize(const Swapchain& swapchain)
    {
        cleanUp();
        allocCommandBuffers(swapchain.getImageCount());
        createPipelines(swapchain);
    }

    void MasterRenderer::submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix)
    {
        _testRenderer.submit(pRenderable, transformationMatrix);
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
        render::begin_render_pass(currentCommandBuffer, swapchain, { 0, 0, 1, 1 }, true);

        // NOTE: We create new copies of secondary command buffers here!
        // Fucking stupid, since the actual command buffers we are using/refering to lives inside the
        // renderer producing the secondary command buffer.
        // TODO: Figure out some nice way to optimize this!
        std::vector<CommandBuffer> secondaryCommandBuffers;

        // TESTING projection matrices
        /*
        Matrix4f projectionMatrix = create_orthographic_projection_matrix(
            -1.0f,
            1.0f,
            -1.0f,
            1.0f,
            0.1f,
            1000.0f
        );*/
        Matrix4f projectionMatrix = create_perspective_projection_matrix(
            800.0f / 600.0f,
            1.3f,
            0.1f,
            100.0f
        );

        const Extent2D swapchainExtent = swapchain.getExtent();
        secondaryCommandBuffers.push_back(
            _testRenderer.recordCommandBuffer(
                swapchain.getRenderPass(),
                swapchainExtent.width,
                swapchainExtent.height,
                projectionMatrix,
                frame // NOTE: no idea should this be the "frame index" or "image index"
            )
        );

        render::exec_secondary_command_buffers(currentCommandBuffer, secondaryCommandBuffers);

        render::end_render_pass(currentCommandBuffer);
        currentCommandBuffer.end();

        return currentCommandBuffer;
    }
}
