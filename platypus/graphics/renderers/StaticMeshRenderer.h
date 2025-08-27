#pragma once

#include "Renderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/assets/Mesh.h"
#include "platypus/assets/Material.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Lights.h"
#include <cstdlib>


namespace platypus
{
    class StaticMeshRenderer : public Renderer
    {
    private:
        struct BatchData
        {
            ID_t identifier = NULL_ID;
            Material* pMaterial = nullptr;
            const Buffer* pVertexBuffer = nullptr;
            const Buffer* pIndexBuffer = nullptr;
            Buffer* pInstancedBuffer = nullptr;
            size_t count = 0;
        };

        std::vector<BatchData> _batches;
        std::unordered_map<ID_t, size_t> _identifierBatchMapping;

        static size_t s_maxBatches;
        static size_t s_maxBatchLength;

    public:
        StaticMeshRenderer(
            const MasterRenderer& masterRenderer,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        ~StaticMeshRenderer();

        virtual void freeBatches();

        virtual void submit(const Scene* pScene, entityID_t entity);

        virtual const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const DescriptorSet& commonDescriptorSet,
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
            ID_t materialID,
            ID_t identifier
        );
    };
}
