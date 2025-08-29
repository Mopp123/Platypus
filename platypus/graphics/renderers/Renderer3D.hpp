#pragma once

#include "platypus/graphics/Pipeline.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/graphics/RenderPass.h"


namespace platypus
{
    // How is batch provided for Renderer3D?
    //  Some batching func somewhere else?
    struct Batch
    {
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
        ShaderStageFlagBits pushConstantsShaderStage;
        uint32_t repeatCount = 1;
        uint32_t instanceCount = 1;
    };


    class MasterRenderer;
    class Renderer3D
    {
    private:
        MasterRenderer& _masterRendererRef;

        std::vector<Batch*> _batches;
        size_t _maxBatches = 0;
        size_t _usedBatches = 0;
        std::vector<CommandBuffer> _commandBuffers;

        size_t _currentFrame = 0;

    public:
        Renderer3D(MasterRenderer& masterRendererRef);
        ~Renderer3D();

        // NOTE: The batch MUST BE COMPLETE when submitting!
        //  -> Meaning that, after this point, the batch shouldn't be modified,
        //  for examply, by adding another entity's data to it.
        void submit(Batch* pBatch);

        CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight
        );

    private:
        void allocCommandBuffers();
        void freeCommandBuffers();
    };
}
