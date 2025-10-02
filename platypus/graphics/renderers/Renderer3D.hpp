#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "Batch.hpp"


namespace platypus
{
    class MasterRenderer;
    class Renderer3D
    {
    private:
        MasterRenderer& _masterRendererRef;

        std::vector<CommandBuffer> _commandBuffers;
        std::vector<CommandBuffer> _offscreenCommandBuffers;
        size_t _currentFrame = 0;

    public:
        Renderer3D(MasterRenderer& masterRendererRef);
        ~Renderer3D();

        // NOTE: Just experimenting atm
        // TODO: Clean this shit up
        CommandBuffer& recordOffscreenCommandBuffer(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const std::vector<Batch*>& toRender
        );

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
