#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "TestRenderer.h"


namespace platypus
{
    class MasterRenderer
    {
    private:
        CommandPool& _commandPoolRef;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        TestRenderer _testRenderer;

    public:
        MasterRenderer(
            const Swapchain& swapchain,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool
        );
        ~MasterRenderer();

        void createPipelines(const Swapchain& swapchain);
        void destroyPipelines();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        void cleanUp();
        void handleWindowResize(const Swapchain& swapchain);

        void submit(const Mesh* pMesh, const Matrix4f& transformationMatrix);

        const CommandBuffer& recordCommandBuffer(const Swapchain& swapchain, size_t frame);
    };
}
