#include "TestRenderer.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cmath>


namespace platypus
{
    static size_t s_TEST_MAX_ENTITIES = 100;
    TestRenderer::TestRenderer(
        const Swapchain& swapchain,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool
    ) :
        _commandPoolRef(commandPool),
        _descriptorPoolRef(descriptorPool),
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
        ),
        _textureDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 2 } }
                }
            }
        )
    {

        size_t uniformBufferElementSize = get_dynamic_uniform_buffer_element_size(sizeof(Matrix4f));
        std::vector<Matrix4f> uniformBufferData(s_TEST_MAX_ENTITIES);
        for (int i = 0; i < swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                _commandPoolRef,
                uniformBufferData.data(),
                uniformBufferElementSize,
                uniformBufferData.size(),
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

        _textureDescriptorSetLayout.destroy();
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
                { 1, ShaderDataType::Float2 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
        std::vector<VertexBufferLayout> vertexBufferLayouts = { vbLayout };
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = {
            &_testDescriptorSetLayout,
            &_textureDescriptorSetLayout
        };

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
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_LESS,
            false, // enable color blending
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void TestRenderer::destroyPipeline()
    {
        _pipeline.destroy();
    }

    void TestRenderer::submit(const StaticMeshRenderable* pRenderable, const Matrix4f& transformationMatrix)
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Mesh* pMesh = assetManager.getMesh(pRenderable->meshID);
        // NOTE: Below just testing... SLOW AS FUCK!
        if (_texDescriptorSetCache.find(pRenderable->textureID) != _texDescriptorSetCache.end())
        {
            _renderList.push_back(
                {
                    pMesh->getVertexBuffer(),
                    pMesh->getIndexBuffer(),
                    transformationMatrix,
                    _texDescriptorSetCache[pRenderable->textureID]
                }
            );
        }
        else
        {
            const Texture* pTexture = assetManager.getTexture(pRenderable->textureID);
            for (int i = 0; i < pApp->getSwapchain().getMaxFramesInFlight(); ++i)
            {
                _texDescriptorSetCache[pRenderable->textureID].push_back(
                    _descriptorPoolRef.createDescriptorSet(
                        &_textureDescriptorSetLayout,
                        { pTexture }
                    )
                );
            }
            Debug::log("___TEST___CREATED NEW DESCRIPTOR SETS!");
            _renderList.push_back(
                {
                    pMesh->getVertexBuffer(),
                    pMesh->getIndexBuffer(),
                    transformationMatrix,
                    _texDescriptorSetCache[pRenderable->textureID]
                }
            );
        }
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
            std::vector<DescriptorSet> descriptorSetsToBind = { _testDescriptorSets[frame], renderData.descriptorSets[frame] };
            std::vector<uint32_t> descriptorSetOffsets = { (uint32_t)uniformBufferOffset };
            render::bind_descriptor_sets(
                currentCommandBuffer,
                descriptorSetsToBind,
                descriptorSetOffsets
            );

            render::bind_vertex_buffers(currentCommandBuffer, { renderData.pVertexBuffer });
            render::bind_index_buffer(currentCommandBuffer, renderData.pIndexBuffer);
            render::draw_indexed(currentCommandBuffer, (uint32_t)renderData.pIndexBuffer->getDataLength(), 1);

            uniformBufferOffset += _testUniformBuffer[frame]->getDataElemSize();
        }

        currentCommandBuffer.end();

        // NOTE: Only temporarely clearing this here every frame!
        _renderList.clear();

        return currentCommandBuffer;
    }
}
