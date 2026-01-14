#pragma once

#include "platypus/graphics/Buffers.hpp"
#include "platypus/graphics/Descriptors.hpp"
#include "platypus/graphics/Shader.hpp"
#include "platypus/graphics/CommandBuffer.hpp"
#include "platypus/assets/Texture.hpp"
#include <map>


namespace platypus
{
    enum class PostProcessingStage
    {
        COLOR_PASS,
        HORIZONRAL_BLUR_PASS,
        VERTICAL_BLUR_PASS,
        SCREEN_PASS
    };

    std::string post_processing_stage_to_string(PostProcessingStage stage);

    struct PostProcessingStageData
    {
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
            PostProcessingStage::HORIZONRAL_BLUR_PASS,
            PostProcessingStage::VERTICAL_BLUR_PASS,
            PostProcessingStage::SCREEN_PASS
        };

        std::map<PostProcessingStage, std::vector<CommandBuffer>> _commandBuffers;

        ImageFormat _colorImageFormat;
        RenderPass _intermediatePass;
        std::map<PostProcessingStage, const RenderPass*> _stageRenderPasses;

        std::map<PostProcessingStage, std::pair<Shader*, Shader*>> _stageShaders;

        // * Stage data objects remain alive throughout the lifetime of application
        // -> some members of the stage data objects gets destroyed and recreated!
        std::map<PostProcessingStage, PostProcessingStageData> _stageData;

        TextureSampler _textureSampler;
        std::map<PostProcessingStage, DescriptorSetLayout> _stageDescriptorSetLayouts;

        float _bloomIntensity = 1.0f;

    public:
        PostProcessingRenderer(
            DescriptorPool& descriptorPool,
            const RenderPass* pScreenPass
        );
        ~PostProcessingRenderer();

        CommandBuffer& recordStagePass(
            CommandBuffer& primaryCommandBuffer,
            PostProcessingStage stageType,
            Framebuffer* pFramebuffer,
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

        inline void setBloomIntensity(float intensity) { _bloomIntensity = intensity; }

    private:
        void loadShaders();
        void destroyShaders();

        void createDescriptorSetLayouts();
        void destroyDescriptorSetLayouts();

        bool validateStagesExist(
            const char* callLocation
        ) const;


        bool validateStageDataComplete(
            PostProcessingStage stage,
            std::string& outErrors
        );
    };
}
