#include "TestRenderer.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include <string>
#include <cmath>


namespace platypus
{
    TestRenderer::TestRenderer(CommandPool& commandPool) :
        _commandPoolRef(commandPool),
        _vertexShader("assets/shaders/TestVertexShader.spv", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("assets/shaders/TestFragmentShader.spv", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
    {
        // NOTE: Not sure can buffers exist through the whole lifetime of the app...

        // TESTING!
        float s = 0.5f;
        std::vector<float> vertexData = {
            -s, -s, 1.0f, 0.0f, 0.0f,
            -s, s,  0.0f, 1.0f, 0.0f,
            s, s,   1.0f, 0.0f, 1.0f,
            s, -s,  1.0f, 1.0f, 0.0f
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        _pVertexBuffer = new Buffer(
            _commandPoolRef,
            vertexData.data(),
            sizeof(float),
            vertexData.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );

        _pIndexBuffer = new Buffer(
            _commandPoolRef,
            indices.data(),
            sizeof(uint32_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );
    }

    TestRenderer::~TestRenderer()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    void TestRenderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = _commandPoolRef.allocCommandBuffers(
            count,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void TestRenderer::freeCommandBuffers()
    {
        for (CommandBuffer& buffer : _commandBuffers)
            buffer.free();
        _commandBuffers.clear();
    }

    void TestRenderer::createPipeline(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight
    )
    {
        _viewportWidth = viewportWidth;
        _viewportHeight = viewportHeight;

        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float2 },
                { 1, ShaderDataType::Float3 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
        std::vector<VertexBufferLayout> vertexBufferLayouts = { vbLayout };

        Rect2D viewportScissor = { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight };
        _pipeline.create(
            renderPass,
            vertexBufferLayouts,
            //const std::vector<DescriptorSetLayout>& descriptorLayouts,
            _vertexShader,
            _fragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_NONE,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            false, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS_OR_EQUAL,
            false, // enable color blending
            0, // push constants size
            0 // push constants' stage flags
        );
    }

    void TestRenderer::destroyPipeline()
    {
        _pipeline.destroy();
    }

    const CommandBuffer& TestRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        size_t frame
    )
    {
        if (frame >= _commandBuffers.size())
        {
            Debug::log(
                "@TestRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(frame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _commandBuffers[frame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, _viewportWidth, _viewportHeight, 0.0f, 1.0f);
        Rect2D scissor = { 0, 0, (uint32_t)_viewportWidth, (uint32_t)_viewportHeight };
        render::set_scissor(currentCommandBuffer, scissor);

        render::bind_pipeline(currentCommandBuffer, _pipeline);
        render::bind_vertex_buffers(currentCommandBuffer, { _pVertexBuffer });
        render::bind_index_buffer(currentCommandBuffer, _pIndexBuffer);
        render::draw_indexed(currentCommandBuffer, (uint32_t)_pIndexBuffer->getDataLength(), 1);

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }
}
