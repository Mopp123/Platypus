#include "TestRenderer.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/RenderCommand.h"
#include <string>
#include <cmath>


namespace platypus
{
    static size_t s_TEST_MAX_ENTITIES = 100;
    static size_t s_TEST_MIN_ALIGNMENT = 256;
    TestRenderer::TestRenderer(
        const Swapchain& swapchain,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool
    ) :
        _commandPoolRef(commandPool),
        _vertexShader("assets/shaders/TestVertexShader.spv", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("assets/shaders/TestFragmentShader.spv", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),

        _testDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { 1, ShaderDataType::Mat4 } }
                }
            }
        )
    {

        // Testing ubos
        /*
        Matrix4f transformationMatrix = create_transformation_matrix(
            { 0, 0, 0 },
            { { 0, 1, 0 }, 0.0f },
            { 1, 1, 1 }
        );
        std::vector<Matrix4f> uniformBufferData(s_TEST_MAX_ENTITIES, transformationMatrix);
        */
        s_TEST_MIN_ALIGNMENT = s_TEST_MIN_ALIGNMENT >= sizeof(Matrix4f) ? s_TEST_MIN_ALIGNMENT : sizeof(Matrix4f);
        std::vector<PE_byte> uniformBufferData(s_TEST_MAX_ENTITIES * s_TEST_MIN_ALIGNMENT, 0);
        for (int i = 0; i < swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                _commandPoolRef,
                uniformBufferData.data(),
                s_TEST_MIN_ALIGNMENT,
                uniformBufferData.size() / s_TEST_MIN_ALIGNMENT,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
            );
            _testUniformBuffer.push_back(pUniformBuffer);

            _testDescriptorSets.push_back(
                descriptorPool.createDescriptorSet(
                    &_testDescriptorSetLayout,
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

        _testDescriptorSetLayout.destroy();
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
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = { &_testDescriptorSetLayout };

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

    void TestRenderer::submit(const Mesh* pMesh, const Matrix4f& transformationMatrix)
    {
        _renderList.push_back(
            {
                pMesh->getVertexBuffer(),
                pMesh->getIndexBuffer(),
                transformationMatrix
            }
        );
    }

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

        size_t uniformBufferOffset = 0;
        for (const RenderData& renderData : _renderList)
        {
            _testUniformBuffer[frame]->update(
                (void*)(&renderData.transformationMatrix),
                sizeof(Matrix4f),
                uniformBufferOffset
            );
            // NOTE: Atm just testing here! quite inefficient to alloc this vector again and again every frame!
            // TODO: Optimize!
            std::vector<DescriptorSet> descriptorSetsToBind = { _testDescriptorSets[frame] };
            std::vector<uint32_t> descriptorSetOffsets = { (uint32_t)uniformBufferOffset };
            render::bind_descriptor_sets(
                currentCommandBuffer,
                descriptorSetsToBind,
                descriptorSetOffsets
            );
            uniformBufferOffset += s_TEST_MIN_ALIGNMENT;

            render::bind_vertex_buffers(currentCommandBuffer, { renderData.pVertexBuffer });
            render::bind_index_buffer(currentCommandBuffer, renderData.pIndexBuffer);
            render::draw_indexed(currentCommandBuffer, (uint32_t)renderData.pIndexBuffer->getDataLength(), 1);
        }

        currentCommandBuffer.end();

        // NOTE: Only temporarely clearing this here every frame!
        _renderList.clear();

        return currentCommandBuffer;
    }
}
