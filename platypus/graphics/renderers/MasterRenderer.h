#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/Swapchain.h"
#include "platypus/graphics/Descriptors.h"
#include "TestRenderer.h"

#include "platypus/ecs/components/Renderable.h"


namespace platypus
{
    struct DirLightUniformBufferData
    {
        Vector4f direction = Vector4f(0, 0, 0, 1);
        Vector4f color = Vector4f(1, 1, 1, 1);
    };

    class MasterRenderer
    {
    private:
        Swapchain _swapchain;
        CommandPool _commandPool;
        DescriptorPool _descriptorPool;
        std::vector<CommandBuffer> _primaryCommandBuffers;

        // Shared descriptor sets among multiple renderers
        DirLightUniformBufferData _useDirLightData;
        std::vector<Buffer*> _dirLightUniformBuffer;
        DescriptorSetLayout _dirLightDescriptorSetLayout;
        std::vector<DescriptorSet> _dirLightDescriptorSets;

        TestRenderer _testRenderer;

    public:
        MasterRenderer(const Window& window);
        ~MasterRenderer();

        void cleanUp();
        void submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix);
        void render(const Window& window);

        inline const Swapchain& getSwapchain() const { return _swapchain; }
        inline CommandPool& getCommandPool() { return _commandPool; }

        inline const TestRenderer& getTestRenderer() const { return _testRenderer; }

    private:
        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();
        void createPipelines();
        void destroyPipelines();
        const CommandBuffer& recordCommandBuffer();
        void handleWindowResize();
    };
}
