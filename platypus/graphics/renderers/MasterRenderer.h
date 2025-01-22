#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
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
        MasterRenderer(CommandPool& commandPool);
        ~MasterRenderer();

        void createPipelines(const Swapchain& swapchain);

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        const CommandBuffer& recordCommandBuffer(const Swapchain& swapchain, size_t frame);
    };
}
