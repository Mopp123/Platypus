#pragma once

#include "Renderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/assets/Mesh.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Lights.h"
#include <cstdlib>


namespace platypus
{
    class StaticMeshRenderer : public Renderer
    {
    private:
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
            CommandPool& commandPool,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        ~StaticMeshRenderer();

        virtual void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const DescriptorSetLayout& dirLightDescriptorSetLayout
        );

        virtual void freeBatches();

        virtual void submit(const Scene* pScene, entityID_t entity);

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& perspectiveProjectionMatrix,
            const Matrix4f& orthographicProjectionMatrix,
            const Matrix4f& viewMatrix,
            const DescriptorSet& dirLightDescriptorSet,
            size_t frame
        );

    private:
        // returns index in _batches if already occupied batch found with enough space.
        // returns -1 if no existing batch found.
        int findExistingBatchIndex(ID_t identifier);

        // returns index in _batches if free found
        // returns -1 if no free batch was found.
        int findFreeBatchIndex();

        void addToBatch(
            BatchData& batchData,
            const Matrix4f& transformationMatrix
        );
        bool occupyBatch(
            BatchData& batchData,
            ID_t meshID,
            ID_t textureID
        );

        void freeBatch(BatchData& batch);
    };
}
