#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Buffers.h"
#include <cstdlib>


namespace platypus
{
    class TestRenderer
    {
    private:
        CommandPool& _commandPoolRef;
        std::vector<CommandBuffer> _commandBuffers;
        Pipeline _pipeline;

        Shader _vertexShader;
        Shader _fragmentShader;

        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;

        float _viewportWidth = 0.0f;
        float _viewportHeight = 0.0f;

    public:
        TestRenderer(CommandPool& commandPool);
        ~TestRenderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight
        );
        void destroyPipeline();

        const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            size_t frame
        );
    };
}
