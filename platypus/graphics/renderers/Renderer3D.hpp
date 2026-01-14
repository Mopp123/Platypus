#pragma once

#include "platypus/graphics/CommandBuffer.hpp"
#include "Batch.hpp"
#include <unordered_map>


namespace platypus
{
    class MasterRenderer;
    class Renderer3D
    {
    private:
        MasterRenderer& _masterRendererRef;

        std::unordered_map<RenderPassType, std::vector<CommandBuffer>> _commandBuffers;
        size_t _currentFrame = 0;

    public:
        Renderer3D(MasterRenderer& masterRendererRef);
        ~Renderer3D();

        CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const std::vector<Batch*>& toRender
        );

        void advanceFrame();

        void allocCommandBuffers();
        void freeCommandBuffers();
    };
}
