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
        Pipeline _normalMappingPipeline;

        Shader _vertexShader;
        Shader _fragmentShader;
        Shader _normalMappingVertexShader;
        Shader _normalMappingFragmentShader;

        DescriptorSetLayout _materialDescriptorSetLayout;
        DescriptorSetLayout _materialDescriptorSetLayoutHD;

        struct BatchData
        {
            ID_t identifier = NULL_ID;
            const Buffer* pVertexBuffer = nullptr;
            const Buffer* pIndexBuffer = nullptr;
            Buffer* pInstancedBuffer = nullptr;
            // Material properties are in a single vec4
            //  x = specular strength
            //  y = shininess
            //  z = isShadeless (0.0 or 1.0)
            //  w = don't know yet...
            std::vector<Buffer*> materialUniformBuffers;
            std::vector<DescriptorSet> materialDescriptorSets;
            bool normalMapping = false;
            size_t count = 0;
        };

        std::vector<BatchData> _batches;
        std::unordered_map<ID_t, size_t> _identifierBatchMapping;
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
            const DescriptorSetLayout& cameraDescriptorSetLayout,
            const DescriptorSetLayout& dirLightDescriptorSetLayout
        );

        virtual void destroyPipeline() override;

        virtual void freeBatches();
        virtual void freeDescriptorSets();

        virtual void submit(const Scene* pScene, entityID_t entity);

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& perspectiveProjectionMatrix,
            const Matrix4f& orthographicProjectionMatrix,
            const DescriptorSet& cameraDescriptorSet,
            const DescriptorSet& dirLightDescriptorSet,
            size_t frame
        );

    private:
        BatchData* findExistingBatch(ID_t identifier);
        int findFreeBatchIndex();

        void addToBatch(
            BatchData& batchData,
            const Matrix4f& transformationMatrix
        );
        bool occupyBatch(
            int batchIndex,
            ID_t meshID,
            ID_t identifier
        );

        void createDescriptorSets(BatchData& batchData, ID_t materialID);
        void freeBatchDescriptorSets(BatchData& batchData);
    };
}
