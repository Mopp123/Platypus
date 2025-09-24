#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/Renderable.h"

#include <unordered_map>


namespace platypus
{
    enum class BatchType
    {
        NONE,
        STATIC_INSTANCED,
        SKINNED
    };

    struct Batch
    {
        BatchType type = BatchType::NONE;
        Pipeline* pPipeline = nullptr;
        std::vector<std::vector<DescriptorSet>> descriptorSets;
        // NOTE: Currently should only use a single dynamic uniform buffer (Don't know what happens otherwise:D)
        //std::vector<uint32_t> dynamicDescriptorSetRanges;
        uint32_t dynamicUniformBufferElementSize = 0;
        std::vector<const Buffer*> staticVertexBuffers;
        std::vector<std::vector<Buffer*>> dynamicVertexBuffers;
        const Buffer* pIndexBuffer = nullptr;
        size_t pushConstantsSize = 0;
        std::vector<UniformInfo> pushConstantsUniformInfos;
        void* pPushConstantsData = nullptr;
        // Atm push constants should ONLY be used in vertex shaders!
        ShaderStageFlagBits pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_NONE;
        // Repeat count should be 1 if instanced
        uint32_t repeatCount = 0;
        // NOTE: When initially creating the batch, this has to be 0 since we're going to add the first entry explicitly
        uint32_t instanceCount = 0;
    };

    class MasterRenderer;
    // TODO: A way to update batch descriptor sets if those were changed (count may have changed as well!)
    class Batcher
    {
    private:
        MasterRenderer& _masterRendererRef;

        std::vector<Batch*> _batches;
        std::unordered_map<ID_t, size_t> _identifierBatchMapping;

        size_t _maxStaticBatchLength;
        size_t _maxSkinnedBatchLength;
        size_t _maxTerrainBatchLength;
        size_t _maxSkinnedMeshJoints;

        static DescriptorSetLayout s_jointDescriptorSetLayout;
        static DescriptorSetLayout s_terrainDescriptorSetLayout;

        // NOTE: Currently assuming these are modified frequently -> need one for each frame in flight!
        std::vector<std::vector<Buffer*>> _allocatedBuffers;
        std::vector<std::vector<DescriptorSet>> _descriptorSets;

        // This is fucking stupid!
        // Need to pass all vertex buffers as const ptr, so need a way to refer to the s_allocatedBuffers
        // when modifying it, instead of the Batch struct's member.
        std::unordered_map<ID_t, size_t> _batchBufferMapping;
        std::unordered_map<ID_t, size_t> _batchDescriptorSetMapping;

    public:
        Batcher(
            MasterRenderer& masterRenderer,
            size_t maxStaticBatchLength,
            size_t maxSkinnedBatchLength,
            size_t maxTerrainBatchLength,
            size_t maxSkinnedMeshJoints
        );
        ~Batcher();

        // Returns batch identifier if found, NULL_ID if not.
        // NOTE: Entity might have components for multiple different batches!
        //  -> Need to call for each renderable type separately
        ID_t getBatchID(ID_t meshID, ID_t materialID);

        // Returns batch identifier, if created successfully
        ID_t createStaticBatch(ID_t meshID, ID_t materialID);
        ID_t createSkinnedBatch(ID_t meshID, ID_t materialID);
        ID_t createTerrainBatch(ID_t terrainMeshID, ID_t terrainMaterialID);

        void addToStaticBatch(
            ID_t identifier,
            const Matrix4f& transformationMatrix,
            size_t currentFrame
        );

        void addToSkinnedBatch(
            ID_t identifier,
            void* pJointData,
            size_t jointDataSize,
            size_t currentFrame
        );

        void addToTerrainBatch(
            ID_t identifier,
            const Matrix4f& transformationMatrix,
            float tileSize,
            uint32_t verticesPerRow,
            size_t currentFrame
        );

        void updateDeviceSideBuffers(size_t currentFrame);
        // Clears instance and repeat counts for next round of submits.
        void resetForNextFrame();

        void freeBatches();

        const std::vector<Batch*>& getBatches() const;

        static const DescriptorSetLayout& get_joint_descriptor_set_layout();
        static const DescriptorSetLayout& get_terrain_descriptor_set_layout();
    };
}
