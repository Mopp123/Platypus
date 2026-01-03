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
        std::vector<CommandBuffer> _colorCommandBuffers;
        std::vector<CommandBuffer> _screenCommandBuffers;

        Shader* _pColorVertexShader = nullptr;
        Shader* _pColorFragmentShader = nullptr;
        Shader* _pScreenVertexShader = nullptr;
        Shader* _pScreenFragmentShader = nullptr;

        ImageFormat _colorImageFormat;
        RenderPass _colorPass;
        TextureSampler _textureSampler;
        Texture* _pColorFramebufferAttachment = nullptr;
        Framebuffer* _pColorFramebuffer = nullptr;

        Pipeline* _pColorPipeline = nullptr;
        Pipeline* _pScreenPipeline = nullptr;

        DescriptorSetLayout _descriptorSetLayout;
        std::vector<DescriptorSet> _colorDescriptorSet;
        std::vector<DescriptorSet> _screenDescriptorSet;

    public:
        PostProcessingRenderer(DescriptorPool& descriptorPool);
        ~PostProcessingRenderer();

        CommandBuffer& recordColorPass(
            float viewportWidth,
            float viewportHeight,
            size_t currentFrame
        );
        CommandBuffer& recordScreenPass(
            const RenderPass& screenPass,
            float viewportWidth,
            float viewportHeight,
            size_t currentFrame
        );

        // Records the complete post processing pipeline into primary command buffer.
        //
        // Returns recorded screen pass command buffers
        //  -> we might want to add more stuff to those after post processing
        //  (GUI rendering for example)
        CommandBuffer& recordCommandBuffer(
            CommandBuffer& primaryCommandBuffer,
            const RenderPass& screenPass,
            Framebuffer* pScreenFramebuffer,
            float viewportWidth,
            float viewportHeight,
            size_t currentFrame
        );

        void allocCommandBuffers();
        void freeCommandBuffers();

        void createFramebuffers();
        void destroyFramebuffers();

        void createPipelines(const RenderPass& screenPass);
        void destroyPipelines();

        void createShaderResources(Texture* pSceneColorAttachment);
        void destroyShaderResources();

        inline RenderPass& getColorPass() { return _colorPass; }
        inline Framebuffer* getColorFramebuffer() { return _pColorFramebuffer; }
    };
}
