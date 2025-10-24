#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/assets/Material.h"
#include "platypus/graphics/RenderPass.h"
#include "platypus/ecs/Entity.h"
#include "platypus/ecs/components/Renderable.h"

#include <unordered_map>

// TODO: Some better way to deal with this...
#define PLATYPUS_BATCHER_AVAILABLE_RENDER_PASSES 2

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
        bool requiresDeviceUpdate = false;
    };


    struct Batch
    {
        size_t maxLength = 0;

        Pipeline* pPipeline = nullptr;
        std::vector<std::vector<DescriptorSet>> descriptorSets;
        // NOTE: Currently should only use a single dynamic uniform buffer (Don't know what happens otherwise:D)
        //std::vector<uint32_t> dynamicDescriptorSetRanges;
        uint32_t dynamicUniformBufferElementSize = 0;
        std::vector<const Buffer*> staticVertexBuffers;
        std::vector<std::vector<Buffer*>> dynamicVertexBuffers;
        const Buffer* pIndexBuffer = nullptr;

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
    };

    struct BatchPipelineData
    {
        Shader* pVertexShader = nullptr;
        Shader* pFragmentShader = nullptr;
        Pipeline* pPipeline = nullptr;
        ~BatchPipelineData()
        {
            delete pVertexShader;
            delete pFragmentShader;
            delete pPipeline;
        }
    };

    class MasterRenderer;
    // TODO: A way to update batch descriptor sets if those were changed (count may have changed as well!)
    class Batcher
    {
    private:
        MasterRenderer& _masterRendererRef;
        DescriptorPool& _descriptorPoolRef;

        std::unordered_map<RenderPassType, std::vector<Batch*>> _batches;
        std::unordered_map<RenderPassType, std::unordered_map<ID_t, size_t>> _identifierBatchMapping;

        // Additional batch pipelines that aren't managed elsewhere
        // (for example shadow pass pipelines are managed here)
        std::vector<BatchPipelineData*> _managedPipelineData;
        // NOTE: The ID here can be anything, not just hash(meshID, materialID)
        //  -> when accessing these pipelines you need to know how the ID was originally created!
        //std::unordered_map<RenderPassType, std::unordered_map<ID_t, size_t>> _identifierPipelineDataMapping;

        size_t _maxStaticBatchLength;
        size_t _maxSkinnedBatchLength;
        size_t _maxTerrainBatchLength;
        size_t _maxSkinnedMeshJoints;

        static RenderPassType s_availableRenderPasses[2];
        static DescriptorSetLayout s_jointDescriptorSetLayout;
        static DescriptorSetLayout s_terrainDescriptorSetLayout;

        // NOTE: Currently assuming these are modified frequently -> need one for each frame in flight!
        std::vector<std::vector<BatchShaderResource>> _allocatedShaderResources;
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

        // Returns batch identifier, if created successfully
        /*
        ID_t createBatch(
            ID_t meshID,
            ID_t materialID,
            ComponentType renderableType,
            const RenderPass* pShadowPass,
            size_t shadowPushConstantsSize,
            void* pShadowPushConstants
        );
        */

        BatchPipelineData* createBatchPipelineData(
            const RenderPass* pRenderPass,
            const std::string& vertexShaderFilename,
            const std::string& fragmentShaderFilename,
            const std::vector<VertexBufferLayout>& vertexBufferLayouts,
            const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
            size_t pushConstantsSize,
            ShaderStageFlagBits pushConstantsShaderStage
        );

        void addBatch(RenderPassType renderPassType, ID_t identifier, Batch* pBatch);

        /*
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
            size_t currentFrame
        );
        */

        // This also updates stuff that doesn't need to be done per instance but for whole
        // batch. Material data for example(if some properties have changed).
        void updateDeviceSideBuffers(size_t currentFrame);
        // Clears instance and repeat counts for next round of submits.
        void resetForNextFrame();

        void freeBatches();


        Batch* getBatch(RenderPassType renderPassType, ID_t identifier);
        // Returns all batches for a render pass
        const std::vector<Batch*>& getBatches(RenderPassType renderPassType);
        // Returns batches sharing the same ID for all render passes
        std::vector<Batch*> getBatches(ID_t identifier);

        static const DescriptorSetLayout& get_joint_descriptor_set_layout();
        static const DescriptorSetLayout& get_terrain_descriptor_set_layout();

        BatchShaderResource* getSharedBatchResource(ID_t batchID, size_t resourceIndex);

        void createSharedBatchInstancedBuffers(
            ID_t identifier,
            size_t bufferElementSize,
            size_t maxBatchLength,
            size_t framesInFlight,
            std::vector<Buffer*>& outBuffers
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
        std::vector<std::vector<DescriptorSet>> combineUsedDescriptorSets(
            size_t framesInFlight,
            const std::vector<std::vector<DescriptorSet>>& descriptorSetsToUse
        );

        // Creates dynamic uniform buffers and descriptor sets for the whole batch
        void createBatchShaderResources(
            size_t framesInFlight,
            ID_t batchID,
            size_t maxBatchLength,
            const std::vector<ShaderResourceLayout>& resourceLayouts,
            std::vector<BatchShaderResource>& outResources
        );

        Pipeline* getSuitableManagedPipeline(
            const std::string& vertexShaderFilename,
            const std::string& fragmentShaderFilename,
            const std::vector<VertexBufferLayout>& vertexBufferLayouts,
            const std::vector<DescriptorSetLayout>& descriptorSetLayouts,
            size_t pushConstantsSize,
            ShaderStageFlagBits pushConstantsShaderStage
        );

        // ...dumb I know, just want to make sure...
        bool validateBatchDoesntExist(
            const char* callLocation,
            RenderPassType renderPassType,
            ID_t batchID
        ) const;

        void destroyManagedPipelines();
        void recreateManagedPipelines();

        inline size_t getMaxStaticBatchLength() const { return _maxStaticBatchLength; }
        inline size_t getMaxSkinnedBatchLength() const { return _maxSkinnedBatchLength; }
        inline size_t getMaxTerrainBatchLength() const { return _maxTerrainBatchLength; }
        inline size_t getMaxSkinnedMeshJoints() const { return _maxSkinnedMeshJoints; }

    private:
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
