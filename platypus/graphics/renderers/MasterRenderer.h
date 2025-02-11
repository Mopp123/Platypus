#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "TestRenderer.h"

#include "platypus/ecs/components/Renderable.h"


namespace platypus
{
    class MasterRenderer
    {
    private:
        Swapchain _swapchain;
        CommandPool _commandPool;
        DescriptorPool _descriptorPool;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        TestRenderer _testRenderer;

    public:
        MasterRenderer(const Window& window);
        ~MasterRenderer();

        void cleanUp();
        void submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix);
        void render(const Window& window);

        inline const Swapchain& getSwapchain() const { return _swapchain; }
        inline CommandPool& getCommandPool() { return _commandPool; }

    private:
        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();
        void createPipelines();
        void destroyPipelines();
        const CommandBuffer& recordCommandBuffer();
        void handleWindowResize();
    };
}
