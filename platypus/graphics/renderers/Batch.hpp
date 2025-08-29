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
        std::vector<uint32_t> dynamicDescriptorSetRanges;
        std::vector<const Buffer*> vertexBuffers;
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
        static std::vector<Buffer*> s_allocatedBuffers;
        // This is fucking stupid!
        // Need to pass all vertex buffers as const ptr, so need a way to refer to the s_allocatedBuffers
        // when modifying it, instead of the Batch struct's member.
        static std::unordered_map<ID_t, size_t> s_batchAdditionalBufferMapping;

    public:
        // NOTE: Entity might have components for multiple different batches!
        //  -> Need to call for each renderable type separately
        static Batch* get_batch(ID_t meshID, ID_t materialID);

        static Batch* create_static_batch(ID_t meshID, ID_t materialID);
        // NOTE: Dumb to pass both Batch and the identifier...
        static void add_to_static_batch(Batch* pBatch, ID_t identifier, const Matrix4f& transformationMatrix);

        static void update_device_side_buffers();
        // Clears instance and repeat counts for next round of submits.
        static void reset_for_next_frame();

        static void free_batches();

        static const std::vector<Batch*>& get_batches();
    };
}
