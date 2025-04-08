#pragma once

#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/utils/Maths.h"
#include "platypus/assets/Mesh.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Lights.h"
#include <cstdlib>


namespace platypus
{
    class MasterRenderer;
    class StaticMeshRenderer
    {
    private:
        const MasterRenderer& _masterRendererRef;
        CommandPool& _commandPoolRef;
        std::vector<CommandBuffer> _commandBuffers;
        DescriptorPool& _descriptorPoolRef;
        Pipeline _pipeline;

        Shader _vertexShader;
        Shader _fragmentShader;

        DescriptorSetLayout _textureDescriptorSetLayout;

        struct BatchData
        {
            ID_t identifier = NULL_ID;
            const Buffer* pVertexBuffer = nullptr;
            const Buffer* pIndexBuffer = nullptr;
            Buffer* pInstancedBuffer = nullptr;
            std::vector<DescriptorSet> textureDescriptorSets;
            size_t count = 0;
        };
        std::vector<BatchData> _batches;
        size_t _currentFrame = 0;

        static size_t s_maxBatches;
        static size_t s_maxBatchLength;

    public:
        StaticMeshRenderer(
            const MasterRenderer& masterRenderer,
            const Swapchain& swapchain,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool
        );
        ~StaticMeshRenderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const DescriptorSetLayout& dirLightDescriptorSetLayout
        );
        void destroyPipeline();

        void submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix);

        const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& projectionMatrix,
            const Matrix4f& viewMatrix,
            const DescriptorSet& dirLightDescriptorSet,
            size_t frame
        );

        // returns index in _batches if already occupied batch found with enough space.
        // returns -1 if no existing batch found.
        int findExistingBatchIndex(ID_t identifier);

        // returns index in _batches if free found
        // returns -1 if no free batch was found.
        int findFreeBatchIndex();
    };
}
