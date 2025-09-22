#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderPass.h"
#include "Batch.hpp"
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


    // TODO: A way to update batch descriptor sets if those were changed (count may have changed as well!)
    class BatchPool
    {
    private:
        static std::vector<Batch*> s_batches;
        static std::unordered_map<ID_t, size_t> s_identifierBatchMapping;

        static size_t s_maxStaticBatchLength;
        static size_t s_maxSkinnedBatchLength;
        static size_t s_maxTerrainBatchLength;
        static size_t s_maxJoints;

        static DescriptorSetLayout s_jointDescriptorSetLayout;
        static DescriptorSetLayout s_terrainDescriptorSetLayout;

        // NOTE: Currently assuming these are modified frequently -> need one for each frame in flight!
        static std::vector<std::vector<Buffer*>> s_allocatedBuffers;
        static std::vector<std::vector<DescriptorSet>> s_descriptorSets;

        // This is fucking stupid!
        // Need to pass all vertex buffers as const ptr, so need a way to refer to the s_allocatedBuffers
        // when modifying it, instead of the Batch struct's member.
        static std::unordered_map<ID_t, size_t> s_batchBufferMapping;
        static std::unordered_map<ID_t, size_t> s_batchDescriptorSetMapping;

    public:
        static void init();
        static void destroy();

        // Returns batch identifier if found, NULL_ID if not.
        // NOTE: Entity might have components for multiple different batches!
        //  -> Need to call for each renderable type separately
        static ID_t get_batch_id(ID_t meshID, ID_t materialID);

        // Returns batch identifier, if created successfully
        static ID_t create_static_batch(ID_t meshID, ID_t materialID);
        static ID_t create_skinned_batch(ID_t meshID, ID_t materialID);
        static ID_t create_terrain_batch(ID_t terrainMeshID, ID_t terrainMaterialID);

        static void add_to_static_batch(
            ID_t identifier,
            const Matrix4f& transformationMatrix,
            size_t currentFrame
        );

        static void add_to_skinned_batch(
            ID_t identifier,
            void* pJointData,
            size_t jointDataSize,
            size_t currentFrame
        );

        static void add_to_terrain_batch(
            ID_t identifier,
            const Matrix4f& transformationMatrix,
            float tileSize,
            uint32_t verticesPerRow,
            size_t currentFrame
        );

        static void update_device_side_buffers(size_t currentFrame);
        // Clears instance and repeat counts for next round of submits.
        static void reset_for_next_frame();

        static void free_batches();

        static const std::vector<Batch*>& get_batches();

        static const DescriptorSetLayout& get_joint_descriptor_set_layout();
        static const DescriptorSetLayout& get_terrain_descriptor_set_layout();
    };
}
