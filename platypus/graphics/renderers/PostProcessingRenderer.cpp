#include "PostProcessingRenderer.hpp"
#include "platypus/graphics/Device.hpp"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"


namespace platypus
{
    PostProcessingRenderer::PostProcessingRenderer(DescriptorPool& descriptorPool) :
        _descriptorPoolRef(descriptorPool),
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
        _pVertexShader = new Shader("PostProcessingVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT);
        _pFragmentShader = new Shader("PostProcessingFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT);
    }

    PostProcessingRenderer::~PostProcessingRenderer()
    {
        destroyShaderResources();
        destroyPipeline();

        if (_pVertexShader)
            delete _pVertexShader;
        if (_pFragmentShader)
            delete _pFragmentShader;

        _descriptorSetLayout.destroy();
        freeCommandBuffers();
    }

    // TODO: This should probably own its' render pass?
    CommandBuffer& PostProcessingRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        size_t currentFrame
    )
    {
        Debug::log("___TEST___PostProcessingRenderer::recordCommandBuffer");
        if (currentFrame >= _commandBuffers.size())
        {
            Debug::log(
                "@PostProcessingRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(currentFrame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _commandBuffers[currentFrame];

        currentCommandBuffer.begin(&renderPass);

        render::bind_pipeline(
            currentCommandBuffer,
            *_pPipeline
        );
        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::set_scissor(currentCommandBuffer, { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight });

        render::bind_descriptor_sets(
            currentCommandBuffer,
            { _descriptorSet[currentFrame] },
            { }
        );

        render::draw(currentCommandBuffer, 6);

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }

    void PostProcessingRenderer::allocCommandBuffers()
    {
        _commandBuffers = Device::get_command_pool()->allocCommandBuffers(
            Application::get_instance()->getSwapchain()->getMaxFramesInFlight(),
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void PostProcessingRenderer::freeCommandBuffers()
    {
        for (CommandBuffer& commandBuffer : _commandBuffers)
            commandBuffer.free();

        _commandBuffers.clear();
    }

    void PostProcessingRenderer::createPipeline(const RenderPass& renderPass)
    {
        if (_pPipeline)
        {
            Debug::log(
                "@PostProcessingRenderer::createPipeline "
                "Pipeline already exists!",
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Debug::log("___TEST___@PostProcessingRenderer allocating pipelin");
        _pPipeline = new Pipeline(
            &renderPass,
            { }, // Vertex buffer layouts
            { _descriptorSetLayout },
            _pVertexShader,
            _pFragmentShader,
            CullMode::CULL_MODE_NONE,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // Enable depth test
            false, // enable depth write
            DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
            false, // Enable color blend NOTE: Might actually be required atm
            0, // push constants size
            ShaderStageFlagBits::SHADER_STAGE_NONE // push constants stage flags
        );
        Debug::log("___TEST___@PostProcessingRenderer creating pipeline");
        _pPipeline->create();
    }

    void PostProcessingRenderer::destroyPipeline()
    {
        if (_pPipeline)
        {
            _pPipeline->destroy();
            delete _pPipeline;
        }
        _pPipeline = nullptr;
    }

    void PostProcessingRenderer::createShaderResources(Texture* pSceneColorAttachment)
    {
        // NOTE: Not sure should we even have for each frame in flight here...?
        const size_t framesInFlight = Application::get_instance()->getSwapchain()->getMaxFramesInFlight();
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            _descriptorSet.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    _descriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pSceneColorAttachment }
                    }
                )
            );
        }
    }

    void PostProcessingRenderer::destroyShaderResources()
    {
        _descriptorPoolRef.freeDescriptorSets(_descriptorSet);
        _descriptorSet.clear();
    }
}
