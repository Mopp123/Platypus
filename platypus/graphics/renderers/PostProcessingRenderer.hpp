#pragma once

#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/Descriptors.h"
#include "platypus/graphics/Shader.h"
#include "platypus/graphics/CommandBuffer.h"
#include "platypus/assets/Texture.h"
#include <map>


namespace platypus
{
    enum class PostProcessingStage
    {
        COLOR_PASS,
        SCREEN_PASS
    };

    std::string post_processing_pass_to_string(PostProcessingStage stage);


    struct PostProcessingStageData
    {
        Shader* pVertexShader = nullptr;
        Shader* pFragmentShader = nullptr;
        Pipeline* pPipeline = nullptr;
        Texture* pFramebufferAttachment = nullptr;
        Framebuffer* pFramebuffer = nullptr;

        std::vector<DescriptorSet> descriptorSet;
    };


    class PostProcessingRenderer
    {
    private:
        DescriptorPool& _descriptorPoolRef;
        const std::set<PostProcessingStage> _stageTypes = {
            PostProcessingStage::COLOR_PASS,
            PostProcessingStage::SCREEN_PASS
        };

        std::map<PostProcessingStage, std::vector<CommandBuffer>> _commandBuffers;

        ImageFormat _colorImageFormat;
        RenderPass _colorPass;
        std::map<PostProcessingStage, RenderPass*> _stageRenderPasses;

        // * Stage data objects remain alive throughout the lifetime of application
        // -> some members of the stage data objects gets destroyed and recreated!
        std::map<PostProcessingStage, PostProcessingStageData> _stageData;

        TextureSampler _textureSampler;
        DescriptorSetLayout _descriptorSetLayout;

    public:
        PostProcessingRenderer(
            DescriptorPool& descriptorPool,
            RenderPass* pScreenPass
        );
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

    private:
        CommandBuffer& recordCommandBuffer(
            PostProcessingStage stage,
            float viewportWidth,
            float viewportHeight,
            size_t currentFrame
        );

        void createStageData();

        // Would be nice if PostProcessingStageData's destructor handled this?
        void destroyStageData();

        bool validateStagesExist(
            const char* callLocation
        ) const;
    };
}
