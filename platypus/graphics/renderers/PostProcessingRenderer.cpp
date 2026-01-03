#include "PostProcessingRenderer.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"


namespace platypus
{
    PostProcessingRenderer::PostProcessingRenderer(DescriptorPool& descriptorPool) :
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
        _pColorVertexShader = new Shader(
            "postProcessing/ColorVertexShader",
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        _pColorFragmentShader = new Shader(
            "postProcessing/ColorFragmentShader",
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );

        _pScreenVertexShader = new Shader(
            "postProcessing/ScreenVertexShader",
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT
        );
        _pScreenFragmentShader = new Shader(
            "postProcessing/ScreenFragmentShader",
            ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
        );

        _colorImageFormat = ImageFormat::R8G8B8A8_SRGB;
        _colorPass.create(_colorImageFormat, ImageFormat::NONE);
    }

    PostProcessingRenderer::~PostProcessingRenderer()
    {
        destroyShaderResources();
        destroyPipelines();
        destroyFramebuffers();
        _colorPass.destroy();

        delete _pColorVertexShader;
        delete _pColorFragmentShader;
        delete _pScreenVertexShader;
        delete _pScreenFragmentShader;

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

    void PostProcessingRenderer::allocCommandBuffers()
    {
        size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        _colorCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            framesInFlight,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
        _screenCommandBuffers = Device::get_command_pool()->allocCommandBuffers(
            framesInFlight,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void PostProcessingRenderer::freeCommandBuffers()
    {
        for (CommandBuffer& commandBuffer : _colorCommandBuffers)
            commandBuffer.free();
        for (CommandBuffer& commandBuffer : _screenCommandBuffers)
            commandBuffer.free();

        _colorCommandBuffers.clear();
        _screenCommandBuffers.clear();
    }

    void PostProcessingRenderer::createFramebuffers()
    {
        const Extent2D swapchainExtent = Application::get_instance()->getSwapchain()->getExtent();
        _pColorFramebufferAttachment = new Texture(
            TextureType::COLOR_TEXTURE,
            _textureSampler,
            _colorImageFormat,
            swapchainExtent.width,
            swapchainExtent.height
        );

        _pColorFramebuffer = new Framebuffer(
            _colorPass,
            { _pColorFramebufferAttachment },
            nullptr,
            swapchainExtent.width,
            swapchainExtent.height
        );
    }

    void PostProcessingRenderer::destroyFramebuffers()
    {
        delete _pColorFramebufferAttachment;
        _pColorFramebufferAttachment = nullptr;

        delete _pColorFramebuffer;
        _pColorFramebuffer = nullptr;
    }

    void PostProcessingRenderer::createPipelines(const RenderPass& screenPass)
    {
        if (_pColorPipeline)
        {
            Debug::log(
                "@PostProcessingRenderer::createPipeline "
                "Color pipeline already exists!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (_pScreenPipeline)
        {
            Debug::log(
                "@PostProcessingRenderer::createPipeline "
                "Screen pipeline already exists!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // TODO: Don't recrete pipelines on window resize
        // -> unnecessary since using dynamic viewport atm!
        _pColorPipeline = new Pipeline(
            &_colorPass,
            { }, // Vertex buffer layouts
            { _descriptorSetLayout },
            _pColorVertexShader,
            _pColorFragmentShader,
            CullMode::CULL_MODE_NONE, // TODO: Cull plz?
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            false, // enable depth write
            DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
            false, // Enable color blend NOTE: Might actually be required atm
            0, // push constants size
            ShaderStageFlagBits::SHADER_STAGE_NONE // push constants stage flags
        );
        _pColorPipeline->create();

        _pScreenPipeline = new Pipeline(
            &screenPass,
            { }, // Vertex buffer layouts
            { _descriptorSetLayout },
            _pScreenVertexShader,
            _pScreenFragmentShader,
            CullMode::CULL_MODE_NONE, // TODO: Cull plz?
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            false, // enable depth write
            DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
            false, // Enable color blend NOTE: Might actually be required atm
            0, // push constants size
            ShaderStageFlagBits::SHADER_STAGE_NONE // push constants stage flags
        );
        _pScreenPipeline->create();
    }

    void PostProcessingRenderer::destroyPipelines()
    {
        if (_pColorPipeline)
        {
            _pColorPipeline->destroy();
            delete _pColorPipeline;
            _pColorPipeline = nullptr;
        }

        if (_pScreenPipeline)
        {
            _pScreenPipeline->destroy();
            delete _pScreenPipeline;
            _pScreenPipeline = nullptr;
        }
    }

    void PostProcessingRenderer::createShaderResources(Texture* pSceneColorAttachment)
    {
        if (!_pColorFramebuffer)
        {
            Debug::log(
                "@PostProcessingRenderer::createShaderResources "
                "Color framebuffer was nullptr. Color framebuffer is required to create "
                "post processing shader resources!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        if (_pColorFramebuffer->getColorAttachments().empty())
        {
            Debug::log(
                "@PostProcessingRenderer::createShaderResources "
                "Color framebuffer's color attachments were empty!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // NOTE: Not sure should we even have for each frame in flight here...?
        const size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            _colorDescriptorSet.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    _descriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pSceneColorAttachment }
                    }
                )
            );

            _screenDescriptorSet.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    _descriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _pColorFramebufferAttachment }
                    }
                )
            );
        }
    }

    void PostProcessingRenderer::destroyShaderResources()
    {
        _descriptorPoolRef.freeDescriptorSets(_colorDescriptorSet);
        _descriptorPoolRef.freeDescriptorSets(_screenDescriptorSet);
        _colorDescriptorSet.clear();
        _screenDescriptorSet.clear();
    }
}
