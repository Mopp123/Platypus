#include "TestRenderer.h"
#include "platypus/core/Debug.h"
#include <string>


namespace platypus
{
    TestRenderer::TestRenderer(CommandPool& commandPool) :
        _commandPoolRef(commandPool),
        _vertexShader("assets/shaders/TestVertexShader.spv", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("assets/shaders/TestFragmentShader.spv", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
    {
        // NOTE: Not sure can buffers exist through the whole lifetime of the app...

        // TESTING!
        float s = 10.0f;
        std::vector<float> positions = {
            -s, -s,
            -s, s,
            s, s,
            s, -s
        };
        std::vector<uint32_t> indices = {
            0, 1, 3,
            3, 1, 2
        };

        _pVertexBuffer = new Buffer(
            positions.data(),
            sizeof(float),
            positions.size(),
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );

        _pIndexBuffer = new Buffer(
            indices.data(),
            sizeof(uint32_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
    }

    TestRenderer::~TestRenderer()
    {
        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    void TestRenderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = _commandPoolRef.allocCommandBuffers(count, CommandBufferLevel::SECONDARY_COMMAND_BUFFER);
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

        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float3 }
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

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }
}
