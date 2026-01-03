#include "PostProcessingRenderer.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"


namespace platypus
{
    static std::string get_post_processing_stage_shader_name(
        PostProcessingStage postProcessingStage,
        ShaderStageFlagBits shaderStage
    )
    {
        std::string shaderName;
        if (postProcessingStage == PostProcessingStage::COLOR_PASS)
            shaderName += "Color";
        else if (postProcessingStage == PostProcessingStage::SCREEN_PASS)
            shaderName += "Screen";

        if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT)
            shaderName += "VertexShader";
        else if (shaderStage == ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
            shaderName += "FragmentShader";

        return shaderName;
    }

    std::string post_processing_pass_to_string(PostProcessingStage stage)
    {
        switch (stage)
        {
            case PostProcessingStage::COLOR_PASS: return "COLOR_PASS";
            case PostProcessingStage::SCREEN_PASS: return "SCREEN_PASS";
        }
    }


    PostProcessingRenderer::PostProcessingRenderer(
        DescriptorPool& descriptorPool,
        RenderPass* pScreenPass
    ) :
        _descriptorPoolRef(descriptorPool),
        _colorPass(
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
        ),
        _descriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    {
                        { }
                    }
                }
            }
        )
    {
        _colorImageFormat = ImageFormat::R8G8B8A8_SRGB;
        _colorPass.create(_colorImageFormat, ImageFormat::NONE);
        _stageRenderPasses[PostProcessingStage::COLOR_PASS] = &_colorPass;
        _stageRenderPasses[PostProcessingStage::SCREEN_PASS] = pScreenPass;

        createStageData();
    }

    PostProcessingRenderer::~PostProcessingRenderer()
    {
        destroyShaderResources();
        destroyPipelines();
        destroyFramebuffers();
        _colorPass.destroy();

        destroyStageData();

        _descriptorSetLayout.destroy();
        freeCommandBuffers();
    }

    CommandBuffer& PostProcessingRenderer::recordColorPass(
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        if (currentFrame >= _colorCommandBuffers.size())
        {
            Debug::log(
                "@PostProcessingRenderer::recordColorPass "
                "Frame index(" + std::to_string(currentFrame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_colorCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _colorCommandBuffers[currentFrame];

        currentCommandBuffer.begin(&_colorPass);

        render::bind_pipeline(
            currentCommandBuffer,
            *_pColorPipeline
        );
        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::set_scissor(currentCommandBuffer, { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight });

        render::bind_descriptor_sets(
            currentCommandBuffer,
            { _colorDescriptorSet[currentFrame] },
            { }
        );

        render::draw(currentCommandBuffer, 6);

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    CommandBuffer& PostProcessingRenderer::recordScreenPass(
        const RenderPass& screenPass,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        if (currentFrame >= _screenCommandBuffers.size())
        {
            Debug::log(
                "@PostProcessingRenderer::recordScreenPass "
                "Frame index(" + std::to_string(currentFrame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_screenCommandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _screenCommandBuffers[currentFrame];

        currentCommandBuffer.begin(&screenPass);

        render::bind_pipeline(
            currentCommandBuffer,
            *_pScreenPipeline
        );
        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::set_scissor(currentCommandBuffer, { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight });

        render::bind_descriptor_sets(
            currentCommandBuffer,
            { _screenDescriptorSet[currentFrame] },
            { }
        );

        render::draw(currentCommandBuffer, 6);

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    CommandBuffer& PostProcessingRenderer::recordCommandBuffer(
        CommandBuffer& primaryCommandBuffer,
        const RenderPass& screenPass,
        Framebuffer* pScreenFramebuffer,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        const Vector4f clearColor(1, 1, 0, 1);

        render::begin_render_pass(
            primaryCommandBuffer,
            _colorPass,
            _pColorFramebuffer,
            clearColor
        );
        std::vector<CommandBuffer> colorPassCommandBuffers;
        colorPassCommandBuffers.push_back(
            recordColorPass(
                viewportWidth,
                viewportHeight,
                currentFrame
            )
        );
        render::exec_secondary_command_buffers(
            primaryCommandBuffer,
            colorPassCommandBuffers
        );
        render::end_render_pass(
            primaryCommandBuffer,
            _colorPass
        );

        // transition color pass output for screen pass
        transition_image_layout(
            primaryCommandBuffer,
            _pColorFramebufferAttachment,
            ImageLayout::SHADER_READ_ONLY_OPTIMAL, // new layout
            PipelineStage::COLOR_ATTACHMENT_OUTPUT_BIT, // src stage
            MemoryAccessFlagBits::MEMORY_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // src access mask
            PipelineStage::FRAGMENT_SHADER_BIT, // dst stage
            MemoryAccessFlagBits::MEMORY_ACCESS_SHADER_READ_BIT // dst access mask
        );


        render::begin_render_pass(
            primaryCommandBuffer,
            screenPass,
            pScreenFramebuffer,
            clearColor
        );

        return recordScreenPass(
            screenPass,
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
            _colorPass,
            { pColorStageAttachment },
            nullptr,
            swapchainExtent.width,
            swapchainExtent.height
        );

        _stageData[PostProcessingStage::COLOR_PASS].pFramebufferAttachment = pColorStageAttachment;
        _stageData[PostProcessingStage::COLOR_PASS].pFramebuffer = pColorStageFramebuffer;
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
                    "Pipeline already exists for post processing stage: " + post_processing_pass_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }
            PostProcessingStageData& stageData = _stageData[stage];

            std::map<PostProcessingStage, RenderPass*>::iterator passIt = _stageRenderPasses.find(stage);
            if (passIt == _stageRenderPasses.end())
            {
                Debug::log(
                    "@PostProcessingRenderer::createPipeline "
                    "No render pass found for post processing stage: " + post_processing_pass_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
                return;
            }

            // TODO: Don't recrete pipelines on window resize
            // -> unnecessary since using dynamic viewport atm!
            stageData.pPipeline = new Pipeline(
                passIt->second,
                { }, // Vertex buffer layouts
                { _descriptorSetLayout },
                stageData.pVertexShader,
                stageData.pFragmentShader,
                CullMode::CULL_MODE_NONE, // TODO: Cull plz?
                FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
                true, // Enable depth test
                false, // enable depth write
                DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
                false, // Enable color blend NOTE: Might actually be required atm
                0, // push constants size
                ShaderStageFlagBits::SHADER_STAGE_NONE // push constants stage flags
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
            PostProcessingStageData& stageData = it->second;
            Texture* pDescriptorSetTexture = nullptr;
            if (it->first == PostProcessingStage::COLOR_PASS)
            {
                pDescriptorSetTexture = pSceneColorAttachment;
            }
            else if (it->first == PostProcessingStage::COLOR_PASS)
            {
                // IMPORTANT! TESTING ONLY!
                //  -> Eventually next post processing stage should take the previous one's
                //  attachment in its descriptor set
                pDescriptorSetTexture = _stageData[PostProcessingStage::COLOR_PASS].pFramebufferAttachment;
            }

            for (size_t i = 0; i < framesInFlight; ++i)
            {
                stageData.descriptorSet.push_back(
                    _descriptorPoolRef.createDescriptorSet(
                        _descriptorSetLayout,
                        {
                            { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pDescriptorSetTexture }
                        }
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

    CommandBuffer& PostProcessingRenderer::recordCommandBuffer(
        PostProcessingStage stage,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        // TODO: continue here!
    }

    void PostProcessingRenderer::createStageData()
    {
        _stageData[PostProcessingStage::COLOR_PASS] = {
            new Shader(
                get_post_processing_stage_shader_name(PostProcessingStage::COLOR_PASS, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
            ),
            new Shader(
                get_post_processing_stage_shader_name(PostProcessingStage::COLOR_PASS, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
            )
        };


        _stageData[PostProcessingStage::SCREEN_PASS] = {
            new Shader(
                get_post_processing_stage_shader_name(PostProcessingStage::SCREEN_PASS, ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
                ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
            ),
            new Shader(
                get_post_processing_stage_shader_name(PostProcessingStage::SCREEN_PASS, ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
                ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
            )
        };
    }

    // Would be nice if PostProcessingStageData's destructor handled this?
    void PostProcessingRenderer::destroyStageData()
    {
        std::map<PostProcessingStage, PostProcessingStageData>::iterator it;
        for (it = _stageData.begin(); it != _stageData.end(); ++it)
        {
            PostProcessingStageData& stageData = it->second;
            if (stageData.pVertexShader)
                delete stageData.pVertexShader;
            if (stageData.pFragmentShader)
                delete stageData.pFragmentShader;

            if (stageData.pPipeline)
            {
                stageData.pPipeline->destroy();
                delete stageData.pPipeline;
            }
            if (stageData.pFramebufferAttachment)
                delete stageData.pFramebufferAttachment;

            if (stageData.pFramebuffer)
                delete stageData.pFramebuffer;

            if (!stageData.descriptorSet.empty())
            {
                _descriptorPoolRef.freeDescriptorSets(stageData.descriptorSet);
                stageData.descriptorSet.clear();
            }

        }
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
                    "No PostProcessingStageData exits for post processing stage: " + post_processing_pass_to_string(stage),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                stagesExist = false;
            }
        }
        if (!stagesExist)
            PLATYPUS_ASSERT(false);

        return stagesExist;
    }
}
