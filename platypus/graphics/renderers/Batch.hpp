#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/assets/Material.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/Renderable.h"

#include <unordered_map>


namespace platypus
{
    enum class ShaderResourceType
    {
        ANY,
        MATERIAL
    };

    // TODO: Maybe better name, since it's not just a layout (takes in Texture ptrs!)
    struct ShaderResourceLayout
    {
        ShaderResourceType type;
        size_t uniformBufferElementSize;
        const DescriptorSetLayout descriptorSetLayout;
        std::vector<Texture*> textures;
    };

    struct BatchShaderResource
    {
        ShaderResourceType type;
        // *Per each frame in flight
        std::vector<Buffer*> buffer;
        std::vector<DescriptorSet> descriptorSet;
    };

    enum class BatchType
    {
        NONE,
        STATIC_INSTANCED,
        SKINNED,
        TERRAIN
    };

    struct Batch
    {
        BatchType type = BatchType::NONE;
        Pipeline* pPipeline = nullptr;
        Pipeline* pOffscreenPipeline = nullptr;
        std::vector<std::vector<DescriptorSet>> descriptorSets;
        // NOTE: Currently should only use a single dynamic uniform buffer (Don't know what happens otherwise:D)
        //std::vector<uint32_t> dynamicDescriptorSetRanges;
        uint32_t dynamicUniformBufferElementSize = 0;
        std::vector<const Buffer*> staticVertexBuffers;
        std::vector<std::vector<Buffer*>> dynamicVertexBuffers;
        const Buffer* pIndexBuffer = nullptr;
        // NOTE: Push constants don't work with the current system
        // TODO: How to get the actual data for the push constants??
        size_t pushConstantsSize = 0;
        std::vector<UniformInfo> pushConstantsUniformInfos;
        void* pPushConstantsData = nullptr;
        // Atm push constants should ONLY be used in vertex shaders!
        ShaderStageFlagBits pushConstantsShaderStage = ShaderStageFlagBits::SHADER_STAGE_NONE;
        // Repeat count should be 1 if instanced
        uint32_t repeatCount = 0;
        // NOTE: When initially creating the batch, this has to be 0 since we're going to add the first entry explicitly
        uint32_t instanceCount = 0;
        ID_t materialAssetID = NULL_ID;
    };

    class MasterRenderer;
    // TODO: A way to update batch descriptor sets if those were changed (count may have changed as well!)
    class Batcher
    {
    private:
        MasterRenderer& _masterRendererRef;
        DescriptorPool& _descriptorPoolRef;

        std::vector<Batch*> _batches;
        std::unordered_map<ID_t, size_t> _identifierBatchMapping;

        size_t _maxStaticBatchLength;
        size_t _maxSkinnedBatchLength;
        size_t _maxTerrainBatchLength;
        size_t _maxSkinnedMeshJoints;

        static DescriptorSetLayout s_jointDescriptorSetLayout;
        static DescriptorSetLayout s_terrainDescriptorSetLayout;

        // NOTE: Currently assuming these are modified frequently -> need one for each frame in flight!
        //std::vector<std::vector<Buffer*>> _allocatedBuffers;
        //std::vector<std::vector<DescriptorSet>> _descriptorSets;
        std::vector<std::vector<BatchShaderResource>> _allocatedShaderResources;

        // This is fucking stupid!
        // Need to pass all vertex buffers as const ptr, so need a way to refer to the s_allocatedBuffers
        // when modifying it, instead of the Batch struct's member.
        std::unordered_map<ID_t, size_t> _batchShaderResourceMapping;

    public:
        Batcher(
            MasterRenderer& masterRenderer,
            DescriptorPool& descriptorPool,
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
        ID_t createTerrainBatch(
            ID_t terrainMeshID,
            ID_t materialID,
            const RenderPass& offscreenRenderPass
        );

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

        // This also updates stuff that doesn't need to be done per instance but for whole
        // batch. Material data for example(if some properties have changed).
        void updateDeviceSideBuffers(size_t currentFrame);
        // Clears instance and repeat counts for next round of submits.
        void resetForNextFrame();

        void freeBatches();

        const std::vector<Batch*>& getBatches() const;

        static const DescriptorSetLayout& get_joint_descriptor_set_layout();
        static const DescriptorSetLayout& get_terrain_descriptor_set_layout();

    private:
        void createBatchInstancedBuffers(
            ID_t batchID,
            size_t bufferElementSize,
            size_t maxBatchLength,
            size_t framesInFlight,
            std::vector<Buffer*>& outBuffers
        );

        // Creates dynamic uniform buffers and descriptor sets for the whole batch
        void createBatchShaderResources(
            size_t framesInFlight,
            ID_t batchID,
            size_t maxBatchLength,
            const std::vector<ShaderResourceLayout>& resourceLayouts,
            std::vector<BatchShaderResource>& outResources
        );

        // When fetching the descriptor sets from anywhere, they are stored in
        // a vector containing descriptor set for each frame in flight.
        // This function converts the given descriptor sets in a format that the batch can use,
        // where the outer vector is for each frame in flight and inner vector all the descriptor sets to bind.
        //
        // Give the descriptorSetsToUse in following manner:
        //  {
        //      someDescriptorSets(for each frame in flight),
        //      someOtherDescriptorSets(for each frame in flight),
        //      ...
        //  }
        void combineUsedDescriptorSets(
            size_t framesInFlight,
            const std::vector<std::vector<DescriptorSet>>& descriptorSetsToUse,
            std::vector<std::vector<DescriptorSet>>& outDescriptorSets
        );

        Batch* getBatch(ID_t batchID);
        Buffer* getBatchBuffer(ID_t batchID, size_t resourceIndex, size_t frame);
        // ...dumb I know, just want to make sure...
        bool validateBatchDoesntExist(const char* callLocation, ID_t batchID) const;

        bool validateDescriptorSetCounts(
            const char* callLocation,
            size_t framesInFlight,
            size_t commonDescriptorSetCount,
            size_t materialDescriptorSetCount
        ) const;

        bool validateDescriptorSetCounts(
            const char* callLocation,
            size_t framesInFlight,
            size_t commonDescriptorSetCount,
            size_t batchDescriptorSetCount,
            size_t materialDescriptorSetCount
        ) const;

        void addToAllocatedShaderResources(
            ID_t batchID,
            std::vector<BatchShaderResource>& shaderResources
        );
        // TODO: delete below after fixing static and skinned batches
        void addToAllocatedShaderResources(
            ID_t batchID,
            const std::vector<Buffer*>& buffers,
            const std::vector<DescriptorSet> descriptorSets
        );
    };
}
