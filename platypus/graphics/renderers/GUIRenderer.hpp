#pragma once


#include "platypus/core/Scene.hpp"
#include "platypus/graphics/CommandBuffer.hpp"
#include "platypus/graphics/Buffers.hpp"
#include "platypus/graphics/Descriptors.hpp"
#include "platypus/graphics/Pipeline.hpp"
#include "platypus/graphics/Shader.hpp"
#include "platypus/ecs/components/Renderable.hpp"
#include "platypus/ecs/components/Transform.hpp"
#include <cstdlib>
#include <map>


namespace platypus
{
    class MasterRenderer;
    class GUIRenderer
    {
    private:
        const MasterRenderer& _masterRendererRef;
        std::vector<CommandBuffer> _commandBuffers;
        DescriptorPool& _descriptorPoolRef;

        size_t _currentFrame = 0;
        uint64_t _requiredComponentsMask = 0;

        Shader _vertexShader;
        Shader _imgFragmentShader;
        Shader _fontFragmentShader;

        const Buffer* _pVertexBuffer = nullptr;
        const Buffer* _pIndexBuffer = nullptr;

        DescriptorSetLayout _textureDescriptorSetLayout;

        // Per instance
        struct GUIRenderData
        {
            // xy = position, zw = scale
            Vector4f translation = Vector4f(0, 0, 1, 1);
            Vector2f textureOffset = Vector2f(0, 0);
            Vector4f color = Vector4f(1, 1, 1, 1);
            Vector4f borderColor = Vector4f(1, 1, 1, 1);
            float borderThickness = 0.0f; // Not sure if this single float will cause problems?
        };

        /*
            NOTE: Batching currently works here in following way:
                *While submitting, batch' count increases
                *After recording a single batch' stuff to the command buffer, its count gets set to 0.
                    -> If encountering batch with count 0 in the next round of submits, this batch gets
                    removed from _toRender map
        */
        enum class BatchType
        {
            NONE,
            IMAGE,
            TEXT
        };

        struct BatchData
        {
            BatchType type = BatchType::NONE;
            ID_t textureID = NULL_ID;
            Buffer* pInstancedBuffer = nullptr;
            float textureAtlasRows = 1.0f;
            size_t count = 0;
        };

        // key = layer, value = index to _batches, which batch to use
        std::map<uint32_t, std::set<size_t>> _toRender;
        std::vector<BatchData> _batches;

        // NOTE: Works atm only because these are for textures -> multiple batches may
        // use same texture descriptor sets.
        //
        // BUT! If want to have batch specific descriptor sets, needs some combined identifier
        // for each batch that contains its' type and textureID
        std::unordered_map<ID_t, std::vector<DescriptorSet>> _textureDescriptorSets;


        Pipeline _imgPipeline;
        Pipeline _fontPipeline;

        static size_t s_maxBatches;
        static size_t s_maxBatchLength;

    public:
        GUIRenderer(
            const MasterRenderer& masterRenderer,
            DescriptorPool& descriptorPool,
            uint64_t requiredComponentsMask
        );
        ~GUIRenderer();

        void allocCommandBuffers(uint32_t count);
        void freeCommandBuffers();

        void createPipeline(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight
        );

        void destroyPipeline();

        // NOTE: This also destroys all texture descriptor sets!
        // This should ONLY be called on swapchain image count change or scene switch!
        void freeBatches();
        void freeDescriptorSets();

        void submit(const Scene* pScene, entityID_t entity);

        const CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            uint32_t viewportWidth,
            uint32_t viewportHeight,
            const Matrix4f& orthographicProjectionMatrix,
            size_t frame
        );

        inline uint64_t getRequiredComponentsMask() const { return _requiredComponentsMask; }

    private:
        // requiredBatchDataElements is the amount elements required to fit into the batch (NOT size in bytes!)
        // For plain images its always 1 but for text it varies ()
        //  -> This is to make sure that a string can fit into the batch
        // NOTE: by providing requiredBatchDataElements = 0, you can just probe does batche exist
        // without intention of adding to it!
        int findExistingBatchIndex(
            uint32_t layer,
            ID_t textureID,
            size_t requiredBatchDataElements
        ) const;
        int findFreeBatchIndex(size_t requiredBatchDataElements) const;

        void addToImageBatch(
            BatchData& batchData,
            const GUIRenderable* pRenderable,
            const GUITransform* pTransform
        );
        // NOTE: You need to make sure that the string can fit into the batch before calling this!
        // (Done guaranteed by findExistingBatchIndex and findFreeBatchIndex)
        void addToFontBatch(
            BatchData& batchData,
            const GUIRenderable* pRenderable,
            const GUITransform* pTransform
        );
        bool occupyBatch(
            BatchType batchType,
            size_t batchIndex,
            uint32_t layer,
            ID_t textureID
        );
        bool freeBatch(
            uint32_t layer,
            ID_t textureID
        );

        bool hasDescriptorSets(ID_t textureID) const;
        void createTextureDescriptorSets(ID_t textureID);
        void freeTextureDescriptorSets(ID_t textureID);
    };
}
