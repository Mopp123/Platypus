#pragma once

#include "Renderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/ecs/components/Renderable.h"
#include "platypus/ecs/components/Transform.h"
#include <cstdlib>
#include <map>


namespace platypus
{
    // Per instance
    struct GUIRenderData
    {
        Vector4f translation = Vector4f(0, 0, 1, 1);
        Vector2f textureOffset = Vector2f(0, 0);
    };

    class GUIRenderer : public Renderer
    {
    private:
        Shader _vertexShader;
        Shader _fragmentShader;

        const Buffer* _pVertexBuffer = nullptr;
        const Buffer* _pIndexBuffer = nullptr;

        DescriptorSetLayout _textureDescriptorSetLayout;

        struct BatchData
        {
            ID_t textureID = NULL_ID;
            Buffer* pInstancedBuffer = nullptr;
            std::vector<DescriptorSet> textureDescriptorSets;
            float textureAtlasRows = 1.0f;
            size_t count = 0;
        };

        // key = layer, value = index to _batches, which batch to use
        std::map<uint32_t, std::set<size_t>> _toRender;
        std::vector<BatchData> _batches;

        size_t _currentFrame = 0;

        static size_t s_maxBatches;
        static size_t s_maxBatchLength;

    public:
        GUIRenderer(
            const MasterRenderer& masterRenderer,
            CommandPool& commandPool,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        ~GUIRenderer();

        virtual void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            const DescriptorSetLayout& dirLightDescriptorSetLayout
        );

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
        int findExistingBatchIndex(uint32_t layer, ID_t textureID);

        int findFreeBatchIndex();

        void addToImageBatch(
            BatchData& batchData,
            const GUITransform* pTransform,
            const Vector2f& textureOffset
        );
        void addToFontBatch(
            BatchData& batchData,
            const GUIRenderable* pRenderable,
            const GUITransform* pTransform
        );
        bool occupyBatch(
            size_t batchIndex,
            uint32_t layer,
            ID_t textureID
        );
    };
}
