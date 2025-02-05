#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/utils/Maths.h"
#include "platypus/assets/Mesh.h"
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

        std::vector<Buffer*> _testUniformBuffer;

        DescriptorSetLayout _testDescriptorSetLayout;
        std::vector<DescriptorSet> _testDescriptorSets;

        float _viewportWidth = 0.0f;
        float _viewportHeight = 0.0f;

        // Just quick dumb way to test rendering multiple thigs
        // TODO:
        //  * Batching
        struct RenderData
        {
            const Buffer* pVertexBuffer = nullptr;
            const Buffer* pIndexBuffer = nullptr;
            Matrix4f transformationMatrix = Matrix4f(1.0f);
        };
        std::vector<RenderData> _renderList;

    public:
        TestRenderer(
            const Swapchain& swapchain,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool
        );
        ~TestRenderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight
        );
        void destroyPipeline();

        void submit(const Mesh* pMesh, const Matrix4f& transformationMatrix);

        const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& projectionMatrix,
            size_t frame
        );
    };
}
