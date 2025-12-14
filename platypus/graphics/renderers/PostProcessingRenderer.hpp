#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/assets/Texture.h"


namespace platypus
{
    class PostProcessingRenderer
    {
    private:
        DescriptorPool& _descriptorPoolRef;

        Buffer* _pVertexBuffer = nullptr;
        Buffer* _pIndexBuffer = nullptr;
        std::vector<CommandBuffer> _commandBuffers;

        Shader* _pVertexShader = nullptr;
        Shader* _pFragmentShader = nullptr;
        Pipeline* _pPipeline = nullptr;

        DescriptorSetLayout _descriptorSetLayout;
        std::vector<DescriptorSet> _descriptorSet;

    public:
        PostProcessingRenderer(DescriptorPool& descriptorPool);
        ~PostProcessingRenderer();

        // TODO: This should probably own its' render pass?
        CommandBuffer& recordCommandBuffer(
            const RenderPass& renderPass,
            float viewportWidth,
            float viewportHeight,
            size_t currentFrame
        );

        void allocCommandBuffers();
        void freeCommandBuffers();

        void createPipeline(const RenderPass& renderPass);
        void destroyPipeline();

        void createShaderResources(Texture* pSceneColorAttachment);
        void destroyShaderResources();
    };
}
