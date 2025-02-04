#include "TestRenderer.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include <string>
#include <cmath>


namespace platypus
{
    TestRenderer::TestRenderer(
        const Swapchain& swapchain,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool
    ) :
        _commandPoolRef(commandPool),
        _vertexShader("assets/shaders/TestVertexShader.spv", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("assets/shaders/TestFragmentShader.spv", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT)
    {
        // NOTE: Not sure can buffers exist through the whole lifetime of the app...

        // TESTING!
        float s = 1.0f;
        std::vector<float> vertexData = {
            -s, -s,     1, 0, 0,
            -s, s,      0, 1, 0,
            s, s,       1, 0, 1,
            s, -s,      1, 1, 0
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

        // Testing ubos
        Matrix4f transformationMatrix = create_transformation_matrix(
            { 0, 0, 0 },
            { 1, 1, 1 },
            { { 0, 1, 0 }, 0.0f }
        );

        // WARNING! Probably shouldn't create desc set layouts like this!
        _pTestDescriptorSetLayout = new DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { 1, ShaderDataType::Mat4 } }
                }
            }
        );

        for (int i = 0; i < swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                _commandPoolRef,
                &transformationMatrix,
                sizeof(Matrix4f),
                1,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
            );
            _testUniformBuffer.push_back(pUniformBuffer);

            _testDescriptorSets.push_back(
                descriptorPool.createDescriptorSet(
                    _pTestDescriptorSetLayout,
                    { pUniformBuffer }
                )
            );
        }
    }

    TestRenderer::~TestRenderer()
    {
        for (Buffer* pUniformBuffer : _testUniformBuffer)
            delete pUniformBuffer;
        _testUniformBuffer.clear();

        delete _pTestDescriptorSetLayout;

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
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = { _pTestDescriptorSetLayout };

        Rect2D viewportScissor = { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight };
        _pipeline.create(
            renderPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
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
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void TestRenderer::destroyPipeline()
    {
        _pipeline.destroy();
    }

    static float s_TEST_value = 0.0f;
    static float s_TEST_anim = 0.0f;
    const CommandBuffer& TestRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& projectionMatrix,
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

        //Debug::log("___TEST___using proj mat:");
        //Debug::log(projectionMatrix.toString());

        render::push_constants(
            currentCommandBuffer,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(Matrix4f),
            &projectionMatrix,
            {
                { 0, ShaderDataType::Mat4 }
            }
        );


        //s_TEST_value += 0.01f;
        Matrix4f transformationMatrix = create_transformation_matrix(
            { 0, 1.0f, -5.0f },
            { 10, 10, 10 },
            { { 1, 0, 0 }, 1.57f }
        );

        _testUniformBuffer[frame]->update(&transformationMatrix, sizeof(Matrix4f));
        // NOTE: Atm just testing here! quite inefficient to alloc this vector again and again every frame!
        // TODO: Optimize!
        std::vector<DescriptorSet> descriptorSetsToBind = { _testDescriptorSets[frame] };
        render::bind_descriptor_sets(
            currentCommandBuffer,
            descriptorSetsToBind
        );

        render::bind_vertex_buffers(currentCommandBuffer, { _pVertexBuffer });
        render::bind_index_buffer(currentCommandBuffer, _pIndexBuffer);
        render::draw_indexed(currentCommandBuffer, (uint32_t)_pIndexBuffer->getDataLength(), 1);

        currentCommandBuffer.end();

        return currentCommandBuffer;
    }
}
