#include "PostProcessingRenderer.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.hpp"
#include "platypus/core/Application.hpp"


namespace platypus
{
    std::string post_processing_stage_to_string(PostProcessingStage stage)
    {
        switch (stage)
        {
            case PostProcessingStage::COLOR_PASS: return "COLOR_PASS";
            case PostProcessingStage::HORIZONRAL_BLUR_PASS: return "HORIZONRAL_BLUR_PASS";
            case PostProcessingStage::VERTICAL_BLUR_PASS: return "VERTICAL_BLUR_PASS";
            case PostProcessingStage::SCREEN_PASS: return "SCREEN_PASS";
        }
        return "<Invalid stage>";
    }


    PostProcessingRenderer::PostProcessingRenderer(
        DescriptorPool& descriptorPool,
        const RenderPass* pScreenPass
    ) :
        _descriptorPoolRef(descriptorPool),
        _intermediatePass(
            RenderPassType::POST_PROCESSING_COLOR_PASS,
            true,
            RenderPassAttachmentUsageFlagBits::RENDER_PASS_ATTACHMENT_USAGE_COLOR_DISCRETE,
            RenderPassAttachmentClearFlagBits::RENDER_PASS_ATTACHMENT_CLEAR_COLOR
        ),
        _textureSampler(
            TextureSamplerFilterMode::SAMPLER_FILTER_MODE_NEAR,
            TextureSamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            false,
            0
        )
    {
        _colorImageFormat = ImageFormat::R8G8B8A8_SRGB;
        _intermediatePass.create(_colorImageFormat, ImageFormat::NONE);
        _stageRenderPasses[PostProcessingStage::COLOR_PASS] = &_intermediatePass;
        _stageRenderPasses[PostProcessingStage::HORIZONRAL_BLUR_PASS] = &_intermediatePass;
        _stageRenderPasses[PostProcessingStage::VERTICAL_BLUR_PASS] = &_intermediatePass;
        _stageRenderPasses[PostProcessingStage::SCREEN_PASS] = pScreenPass;

        loadShaders();
        createDescriptorSetLayouts();

        for (PostProcessingStage stage : _stageTypes)
            _stageData[stage] = { };
    }

    PostProcessingRenderer::~PostProcessingRenderer()
    {
        destroyShaderResources();
        destroyPipelines();
        destroyFramebuffers();

        _intermediatePass.destroy();

        destroyShaders();
        destroyDescriptorSetLayouts();

        freeCommandBuffers();
    }

    CommandBuffer& PostProcessingRenderer::recordStagePass(
        CommandBuffer& primaryCommandBuffer,
        PostProcessingStage stageType,
        Framebuffer* pFramebuffer,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        const Vector4f clearColor(1, 1, 0, 1);

        std::vector<CommandBuffer>& stageCommandBuffers = _commandBuffers[stageType];
        if (currentFrame >= stageCommandBuffers.size())
        {
            Debug::log(
                "@PostProcessingRenderer::recordIntermediatePass "
                "Post processing stage's (" + post_processing_stage_to_string(stageType) + " "
                "Command buffer out of bounds. Current frame: " + std::to_string(currentFrame) + ", "
                "command buffer count: " + std::to_string(stageCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        PostProcessingStageData& stageData = _stageData[stageType];
        const RenderPass* pRenderPass = _stageRenderPasses[stageType];
        const std::vector<DescriptorSet>& descriptorSets = stageData.descriptorSet;

        if (currentFrame >= descriptorSets.size())
        {
            Debug::log(
                "@PostProcessingRenderer::recordIntermediatePass "
                "Post processing stage's (" + post_processing_stage_to_string(stageType) + " "
                "Descriptor set out of bounds. Current frame: " + std::to_string(currentFrame) + ", "
                "descriptor set count: " + std::to_string(descriptorSets.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }
        const DescriptorSet& descriptorSet = descriptorSets[currentFrame];

        render::begin_render_pass(
            primaryCommandBuffer,
            *pRenderPass,
            pFramebuffer,
            clearColor
        );

        CommandBuffer& currentCommandBuffer = stageCommandBuffers[currentFrame];
        currentCommandBuffer.begin(pRenderPass);

        render::bind_pipeline(
            currentCommandBuffer,
            *stageData.pPipeline
        );
        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::set_scissor(currentCommandBuffer, { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight });

        // Provide framebuffer's width or height for blur passes
        // Provide bloom intensity for screen pass
        // *All these are a single float
        if (stageType == PostProcessingStage::HORIZONRAL_BLUR_PASS ||
            stageType == PostProcessingStage::VERTICAL_BLUR_PASS ||
            stageType == PostProcessingStage::SCREEN_PASS)
        {
            // NOTE: Not sure if pPushConstantsData "lifetime" issue here...
            float pushConstantValue = 0;
            if (stageType == PostProcessingStage::HORIZONRAL_BLUR_PASS)
                pushConstantValue = pFramebuffer->getWidth();
            else if (stageType == PostProcessingStage::VERTICAL_BLUR_PASS)
                pushConstantValue = pFramebuffer->getHeight();
            else if (stageType == PostProcessingStage::SCREEN_PASS)
                pushConstantValue = _bloomIntensity;

            render::push_constants(
                currentCommandBuffer,
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(float),
                &pushConstantValue,
                { { ShaderDataType::Float} }
            );
        }

        render::bind_descriptor_sets(
            currentCommandBuffer,
            { descriptorSet },
            { }
        );

        render::draw(currentCommandBuffer, 6);
        currentCommandBuffer.end();


        // Don't exec screen stage command buffer yet -> that may be used after post processing
        // Don't end the screen pass since it WILL be used after post processing (at least GUI rendering)
        // Don't transition screen pass img layout -> nobody's sampling it
        if (stageType != PostProcessingStage::SCREEN_PASS)
        {
            render::exec_secondary_command_buffers(
                primaryCommandBuffer,
                { currentCommandBuffer }
            );
            render::end_render_pass(
                primaryCommandBuffer,
                *pRenderPass
            );

            transition_image_layout(
                primaryCommandBuffer,
                stageData.pFramebufferAttachment,
                ImageLayout::SHADER_READ_ONLY_OPTIMAL, // new layout
                PipelineStage::COLOR_ATTACHMENT_OUTPUT_BIT, // src stage
                MemoryAccessFlagBits::MEMORY_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // src access mask
                PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
                MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
            );
        }
        return currentCommandBuffer;
    }

    CommandBuffer& PostProcessingRenderer::recordCommandBuffer(
        CommandBuffer& primaryCommandBuffer,
        Framebuffer* pScreenFramebuffer,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        const Vector4f clearColor(1, 1, 0, 1);

        const std::set<PostProcessingStage> intermediateStages = {
            PostProcessingStage::COLOR_PASS,
            PostProcessingStage::HORIZONRAL_BLUR_PASS,
            PostProcessingStage::VERTICAL_BLUR_PASS
        };
        for (PostProcessingStage stage : intermediateStages)
        {
            #ifdef PLATYPUS_DEBUG
            std::string errorMsg;
            if (!validateStageDataComplete(stage, errorMsg))
            {
                Debug::log(
                    "@PostProcessingRenderer::recordCommandBuffer "
                    "Stage data validation failed with following errors:\n" + errorMsg,
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
            #endif

            Framebuffer* pFramebuffer = _stageData[stage].pFramebuffer;
            recordStagePass(
                primaryCommandBuffer,
                stage,
                pFramebuffer,
                pFramebuffer->getWidth(),
                pFramebuffer->getHeight(),
                currentFrame
            );
        }

        // Return the screen pass command buffer, which will eventually be recorded into the
        // primary command buffer via MasterRenderer
        return recordStagePass(
            primaryCommandBuffer,
            PostProcessingStage::SCREEN_PASS,
            pScreenFramebuffer,
            viewportWidth,
            viewportHeight,
            currentFrame
        );
    }

    void PostProcessingRenderer::allocCommandBuffers()
    {
        size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        for (PostProcessingStage stage : _stageTypes)
        {
            _commandBuffers[stage] = Device::get_command_pool()->allocCommandBuffers(
                framesInFlight,
                CommandBufferLevel::SECONDARY_COMMAND_BUFFER
            );
        }
    }

    void PostProcessingRenderer::freeCommandBuffers()
    {
        std::map<PostProcessingStage, std::vector<CommandBuffer>>::iterator it;
        for (it = _commandBuffers.begin(); it != _commandBuffers.end(); ++it)
        {
            for (CommandBuffer& commandBuffer : it->second)
                commandBuffer.free();

            // unnecessary...
            it->second.clear();
        }
        _commandBuffers.clear();
    }

    void PostProcessingRenderer::createFramebuffers()
    {
        const Extent2D swapchainExtent = Application::get_instance()->getSwapchain()->getExtent();
        if (!validateStagesExist("PostProcessingRenderer::createFramebuffers"))
            return;

        Texture* pColorStageAttachment = new Texture(
            TextureType::COLOR_TEXTURE,
            _textureSampler,
            _colorImageFormat,
            swapchainExtent.width,
            swapchainExtent.height
        );

        Framebuffer* pColorStageFramebuffer = new Framebuffer(
            _intermediatePass,
            { pColorStageAttachment },
            nullptr,
            swapchainExtent.width,
            swapchainExtent.height
        );
        _stageData[PostProcessingStage::COLOR_PASS].pFramebufferAttachment = pColorStageAttachment;
        _stageData[PostProcessingStage::COLOR_PASS].pFramebuffer = pColorStageFramebuffer;


        uint32_t blurFramebufferWidth = swapchainExtent.width / 2;
        uint32_t blurFramebufferHeight = swapchainExtent.height / 2;

        Texture* pHorizontalBlurStageAttachment = new Texture(
            TextureType::COLOR_TEXTURE,
            _textureSampler,
            _colorImageFormat,
            blurFramebufferWidth,
            blurFramebufferHeight
        );

        Framebuffer* pHorizontalBlurStageFramebuffer = new Framebuffer(
            _intermediatePass,
            { pHorizontalBlurStageAttachment },
            nullptr,
            blurFramebufferWidth,
            blurFramebufferHeight
        );
        _stageData[PostProcessingStage::HORIZONRAL_BLUR_PASS].pFramebufferAttachment = pHorizontalBlurStageAttachment;
        _stageData[PostProcessingStage::HORIZONRAL_BLUR_PASS].pFramebuffer = pHorizontalBlurStageFramebuffer;


        Texture* pVerticalBlurStageAttachment = new Texture(
            TextureType::COLOR_TEXTURE,
            _textureSampler,
            _colorImageFormat,
            blurFramebufferWidth,
            blurFramebufferHeight
        );

        Framebuffer* pVerticalBlurStageFramebuffer = new Framebuffer(
            _intermediatePass,
            { pVerticalBlurStageAttachment },
            nullptr,
            blurFramebufferWidth,
            blurFramebufferHeight
        );
        _stageData[PostProcessingStage::VERTICAL_BLUR_PASS].pFramebufferAttachment = pVerticalBlurStageAttachment;
        _stageData[PostProcessingStage::VERTICAL_BLUR_PASS].pFramebuffer = pVerticalBlurStageFramebuffer;
    }

    void PostProcessingRenderer::destroyFramebuffers()
    {
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        for (it = _stageData.begin(); it != _stageData.end(); ++it)
        {
            PostProcessingStageData& stageData = it->second;
            if (stageData.pFramebufferAttachment)
            {
                delete stageData.pFramebufferAttachment;
                stageData.pFramebufferAttachment = nullptr;
            }
            if (stageData.pFramebuffer)
            {
                delete stageData.pFramebuffer;
                stageData.pFramebuffer = nullptr;
            }
        }
    }

    void PostProcessingRenderer::createPipelines(const RenderPass& screenPass)
    {
        if (!validateStagesExist("PostProcessingRenderer::createPipelines"))
            return;

        for (PostProcessingStage stage : _stageTypes)
        {
            if (_stageData[stage].pPipeline)
            {
                Debug::log(
                    "@PostProcessingRenderer::createPipeline "
                    "Pipeline already exists for post processing stage: " + post_processing_stage_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            PostProcessingStageData& stageData = _stageData[stage];

            std::map<PostProcessingStage, const RenderPass*>::iterator passIt = _stageRenderPasses.find(stage);
            if (passIt == _stageRenderPasses.end())
            {
                Debug::log(
                    "@PostProcessingRenderer::createPipeline "
                    "No render pass found for post processing stage: " + post_processing_stage_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            std::map<PostProcessingStage, std::pair<Shader*, Shader*>>::iterator shadersIt = _stageShaders.find(stage);
            if (shadersIt == _stageShaders.end())
            {
                Debug::log(
                    "@PostProcessingRenderer::createPipeline "
                    "No shaders found for post processing stage: " + post_processing_stage_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            Shader* pVertexShader = shadersIt->second.first;
            Shader* pFragmentShader = shadersIt->second.second;

            // Provide framebuffer's width or height for blur passes
            // Provide bloom intensity for screen pass
            // *All these are a single float
            size_t pushConstantsSize = 0;
            ShaderStageFlagBits pushConstantShaderStage = ShaderStageFlagBits::SHADER_STAGE_NONE;
            if (stage == PostProcessingStage::HORIZONRAL_BLUR_PASS ||
                stage == PostProcessingStage::VERTICAL_BLUR_PASS ||
                stage == PostProcessingStage::SCREEN_PASS)
            {
                pushConstantsSize = sizeof(float);
                pushConstantShaderStage = ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT;
            }

            // TODO: Don't recrete pipelines on window resize
            // -> unnecessary since using dynamic viewport atm!
            stageData.pPipeline = new Pipeline(
                passIt->second,
                { }, // Vertex buffer layouts
                { _stageDescriptorSetLayouts[stage] },
                pVertexShader,
                pFragmentShader,
                CullMode::CULL_MODE_BACK, // TODO: Cull plz?
                FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
                true, // Enable depth test
                false, // enable depth write
                DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
                false, // Enable color blend NOTE: Might actually be required atm
                pushConstantsSize,
                pushConstantShaderStage // push constants stage flags
            );
            stageData.pPipeline->create();
        }
    }

    void PostProcessingRenderer::destroyPipelines()
    {
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        for (it = _stageData.begin(); it != _stageData.end(); ++it)
        {
            PostProcessingStageData& stageData = it->second;
            if (stageData.pPipeline)
            {
                // NOTE: Don't delete pPipeline here?
                // -> u could just use the destroy() and create()
                //  -> which also shouldn't be required anymore since using pipeline's
                //  dynamic viewport stage
                stageData.pPipeline->destroy();
                delete stageData.pPipeline;
                stageData.pPipeline = nullptr;
            }
        }
    }

    void PostProcessingRenderer::createShaderResources(Texture* pSceneColorAttachment)
    {
        // NOTE: Not sure should we even have for each frame in flight here...?
        const size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        for (it = _stageData.begin(); it != _stageData.end(); ++it)
        {
            const PostProcessingStage stageType = it->first;
            PostProcessingStageData& stageData = it->second;
            if (_stageDescriptorSetLayouts.find(stageType) == _stageDescriptorSetLayouts.end())
            {
                Debug::log(
                    "@PostProcessingRenderer::createShaderResources "
                    "No descriptor set layout found for post processing stage: " + post_processing_stage_to_string(stageType),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            std::vector<DescriptorSetComponent> descriptorSetComponents;
            if (stageType == PostProcessingStage::COLOR_PASS)
            {
                descriptorSetComponents.push_back(
                    {
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        (const void*)pSceneColorAttachment,
                    }
                );
            }
            else if (stageType == PostProcessingStage::HORIZONRAL_BLUR_PASS)
            {
                descriptorSetComponents.push_back(
                    {
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        _stageData[PostProcessingStage::COLOR_PASS].pFramebufferAttachment
                    }
                );
            }
            else if (stageType == PostProcessingStage::VERTICAL_BLUR_PASS)
            {
                descriptorSetComponents.push_back(
                    {
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        _stageData[PostProcessingStage::HORIZONRAL_BLUR_PASS].pFramebufferAttachment
                    }
                );
            }
            else if (stageType == PostProcessingStage::SCREEN_PASS)
            {
                // IMPORTANT! TESTING ONLY!
                //  -> Eventually next post processing stage should take the previous one's
                //  attachment in its descriptor set
                //pDescriptorSetTexture = _stageData[PostProcessingStage::COLOR_PASS].pFramebufferAttachment;

                descriptorSetComponents.push_back(
                    {
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        _stageData[PostProcessingStage::VERTICAL_BLUR_PASS].pFramebufferAttachment
                    }
                );
                descriptorSetComponents.push_back(
                    {
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        pSceneColorAttachment
                    }
                );
            }

            for (size_t i = 0; i < framesInFlight; ++i)
            {
                stageData.descriptorSet.push_back(
                    _descriptorPoolRef.createDescriptorSet(
                        _stageDescriptorSetLayouts[stageType],
                        descriptorSetComponents
                    )
                );
            }
        }
    }

    void PostProcessingRenderer::destroyShaderResources()
    {
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        for (it = _stageData.begin(); it != _stageData.end(); ++it)
        {
            PostProcessingStageData& stageData = it->second;
            _descriptorPoolRef.freeDescriptorSets(stageData.descriptorSet);
            stageData.descriptorSet.clear();
        }
    }

    void PostProcessingRenderer::loadShaders()
    {
        _stageShaders[PostProcessingStage::COLOR_PASS] = {
            new Shader("postProcessing/ColorVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
            new Shader("postProcessing/ColorFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        };

        // NOTE: Blur shaders share the same fragment shader
        Shader* pBlurFragmentShader = new Shader("postProcessing/BlurFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT);
        _stageShaders[PostProcessingStage::HORIZONRAL_BLUR_PASS] = {
            new Shader("postProcessing/HorizontalBlurVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
            pBlurFragmentShader
        };
        _stageShaders[PostProcessingStage::VERTICAL_BLUR_PASS] = {
            new Shader("postProcessing/VerticalBlurVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
            pBlurFragmentShader
        };
        _stageShaders[PostProcessingStage::SCREEN_PASS] = {
            new Shader("postProcessing/ScreenVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
            new Shader("postProcessing/ScreenFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
        };
    }

    void PostProcessingRenderer::destroyShaders()
    {
        std::map<PostProcessingStage, std::pair<Shader*, Shader*>>::iterator it;
        for (it = _stageShaders.begin(); it != _stageShaders.end(); ++it)
        {
            delete it->second.first;
            // NOTE: Blur shaders share the same fragment shader
            // -> for now horizontal blur stage destroys it (ignore for vertical)
            // TODO: Better way to deal with that!
            if (it->first != PostProcessingStage::VERTICAL_BLUR_PASS)
                delete it->second.second;
        }
        _stageShaders.clear();
    }

    void PostProcessingRenderer::createDescriptorSetLayouts()
    {
        for (PostProcessingStage stage : _stageTypes)
        {
            std::vector<DescriptorSetLayoutBinding> layoutBindings;
            // *screen pass uses scene pass and prev post processing passes' attachments
            uint32_t textureBindings = (stage == PostProcessingStage::SCREEN_PASS) ? 2 : 1;
            for (uint32_t binding = 0; binding < textureBindings; ++binding)
            {
                layoutBindings.push_back(
                    {
                        binding,
                        1,
                        DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                        {
                            { }
                        }
                    }
                );
            }
            _stageDescriptorSetLayouts[stage] = {
                layoutBindings
            };
        }
    }

    void PostProcessingRenderer::destroyDescriptorSetLayouts()
    {
        std::map<PostProcessingStage, DescriptorSetLayout>::iterator descriptorSetLayoutIt;
        for (descriptorSetLayoutIt = _stageDescriptorSetLayouts.begin(); descriptorSetLayoutIt != _stageDescriptorSetLayouts.end(); ++descriptorSetLayoutIt)
            descriptorSetLayoutIt->second.destroy();
    }

    bool PostProcessingRenderer::validateStagesExist(
        const char* callLocation
    ) const
    {
        const std::string locationStr(callLocation);
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        bool stagesExist = true;
        for (PostProcessingStage stage : _stageTypes)
        {
            if (_stageData.find(stage) == _stageData.end())
            {
                Debug::log(
                    "@" + locationStr + " "
                    "No PostProcessingStageData exits for post processing stage: " + post_processing_stage_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                stagesExist = false;
            }
        }
        if (!stagesExist)
            PLATYPUS_ASSERT(false);

        return stagesExist;
    }

    bool PostProcessingRenderer::validateStageDataComplete(
        PostProcessingStage stage,
        std::string& outErrors
    )
    {
        std::map<PostProcessingStage, std::vector<CommandBuffer>>::iterator commandBufferIt = _commandBuffers.find(stage);
        std::map<PostProcessingStage, const RenderPass*>::iterator passIt = _stageRenderPasses.find(stage);
        std::map<PostProcessingStage, PostProcessingStageData>::iterator stageDataIt = _stageData.find(stage);

        const std::string errorHeader = "Stage: " + post_processing_stage_to_string(stage) + "\n";
        std::string errorMsg;
        if (commandBufferIt == _commandBuffers.end())
            errorMsg += "No command buffers exist\n";

        if (passIt == _stageRenderPasses.end())
            errorMsg += "No render pass exist\n";

        if (stageDataIt == _stageData.end())
        {
            errorMsg += "No stage data exists\n";
        }
        else
        {
            const PostProcessingStageData& stageData = stageDataIt->second;

            // *Screen stage is using swapchain's framebuffer
            if (stage != PostProcessingStage::SCREEN_PASS)
            {
                if (!stageData.pFramebufferAttachment)
                    errorMsg += "No framebuffer attachment exists for stage data\n";
                if (!stageData.pFramebuffer)
                    errorMsg += "No framebuffer exists for stage data\n";
            }
            if (!stageData.pPipeline)
                errorMsg += "No pipeline exists for stage data\n";
            if (stageData.descriptorSet.empty())
                errorMsg += "No descriptor sets exists for stage data\n";
        }

        if (!errorMsg.empty())
            outErrors = errorHeader + errorMsg;

        return errorMsg.empty();
    }
}
