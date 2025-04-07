#include "StaticMeshRenderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cmath>


namespace platypus
{
    size_t StaticMeshRenderer::s_maxBatches = 20;
    size_t StaticMeshRenderer::s_maxBatchLength = 100;
    StaticMeshRenderer::StaticMeshRenderer(
        const MasterRenderer& masterRenderer,
        const Swapchain& swapchain,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool
    ) :
        _masterRendererRef(masterRenderer),
        _commandPoolRef(commandPool),
        _descriptorPoolRef(descriptorPool),
        _vertexShader("TestVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("TestFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
        _textureDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 4 } }
                }
            }
        )
    {
        _batches.resize(s_maxBatches);
        for (size_t i = 0; i < _batches.size(); ++i)
        {
            BatchData& batchData = _batches[i];
            std::vector<Matrix4f> transformsBuffer(s_maxBatchLength);
            batchData.pInstancedBuffer = new Buffer(
                _commandPoolRef,
                transformsBuffer.data(),
                sizeof(Matrix4f),
                transformsBuffer.size(),
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
            );

        }
    }

    StaticMeshRenderer::~StaticMeshRenderer()
    {
        for (BatchData& b : _batches)
        {
            delete b.pInstancedBuffer;
            // NOTE: shouldn't we also delete descriptor sets?
        }
        _textureDescriptorSetLayout.destroy();
    }

    void StaticMeshRenderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = _commandPoolRef.allocCommandBuffers(
            count,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void StaticMeshRenderer::freeCommandBuffers()
    {
        for (CommandBuffer& buffer : _commandBuffers)
            buffer.free();
        _commandBuffers.clear();
    }

    void StaticMeshRenderer::createPipeline(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        const DescriptorSetLayout& dirLightDescriptorSetLayout
    )
    {
        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float3 },
                { 1, ShaderDataType::Float3 },
                { 2, ShaderDataType::Float2 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
        VertexBufferLayout instancedVbLayout = {
            {
                { 3, ShaderDataType::Float4 },
                { 4, ShaderDataType::Float4 },
                { 5, ShaderDataType::Float4 },
                { 6, ShaderDataType::Float4 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
            1
        };
        std::vector<VertexBufferLayout> vertexBufferLayouts = { vbLayout, instancedVbLayout };
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = {
            &dirLightDescriptorSetLayout,
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
            sizeof(Matrix4f) * 2, // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void StaticMeshRenderer::destroyPipeline()
    {
        _pipeline.destroy();
    }

    void StaticMeshRenderer::submit(
        const StaticMeshRenderable* pRenderable,
        const Matrix4f& transformationMatrix
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Mesh* pMesh = assetManager.getMesh(pRenderable->meshID);

        ID_t textureID = pRenderable->textureID;
        int foundBatchIndex = findExistingBatchIndex(textureID);
        if (foundBatchIndex != -1)
        {
            // add to existing batch
            BatchData& currentBatch = _batches[foundBatchIndex];

            currentBatch.pInstancedBuffer->update(
                (void*)(&transformationMatrix),
                sizeof(Matrix4f),
                currentBatch.count * sizeof(Matrix4f)
            );
            ++currentBatch.count;
        }
        else
        {
            // occupy new batch
            int freeBatchIndex = findFreeBatchIndex();
            if (freeBatchIndex == -1)
            {
                Debug::log(
                    "@StaticMeshRenderer::submit "
                    "No free batches found!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            BatchData& currentBatch = _batches[freeBatchIndex];
            currentBatch.identifier = textureID;
            currentBatch.pVertexBuffer = pMesh->getVertexBuffer();
            currentBatch.pIndexBuffer = pMesh->getIndexBuffer();

            const Texture* pTexture = assetManager.getTexture(textureID);
            size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
            for (int i = 0; i < maxFramesInFlight; ++i)
            {
                currentBatch.textureDescriptorSets.push_back(
                    _descriptorPoolRef.createDescriptorSet(
                        &_textureDescriptorSetLayout,
                        { pTexture }
                    )
                );
            }

            currentBatch.pInstancedBuffer->update(
                (void*)(&transformationMatrix),
                sizeof(Matrix4f),
                currentBatch.count * sizeof(Matrix4f)
            );
            ++currentBatch.count;
        }
    }

    const CommandBuffer& StaticMeshRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& projectionMatrix,
        const Matrix4f& viewMatrix,
        const DescriptorSet& dirLightDescriptorSet,
        size_t frame
    )
    {
        if (_currentFrame >= _commandBuffers.size())
        {
            Debug::log(
                "@StaticMeshRenderer::recordCommandBuffer "
                "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
        }

        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        Rect2D scissor = { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight };
        //render::set_scissor(currentCommandBuffer, scissor);

        render::bind_pipeline(currentCommandBuffer, _pipeline);

        Matrix4f pushConstants[2] = { projectionMatrix, viewMatrix };
        render::push_constants(
            currentCommandBuffer,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(Matrix4f) * 2,
            pushConstants,
            {
                { 0, ShaderDataType::Mat4 },
                { 1, ShaderDataType::Mat4 }
            }
        );

        for (BatchData& batchData : _batches)
        {
            if (batchData.identifier == NULL_ID)
                continue;

            render::bind_vertex_buffers(
                currentCommandBuffer,
                {
                    batchData.pVertexBuffer,
                    batchData.pInstancedBuffer
                }
            );
            render::bind_index_buffer(currentCommandBuffer, batchData.pIndexBuffer);

            std::vector<DescriptorSet> descriptorSetsToBind = {
                dirLightDescriptorSet,
                batchData.textureDescriptorSets[_currentFrame]
            };

            render::bind_descriptor_sets(
                currentCommandBuffer,
                descriptorSetsToBind,
                { }
            );

            render::draw_indexed(
                currentCommandBuffer,
                (uint32_t)batchData.pIndexBuffer->getDataLength(),
                batchData.count
            );
            // "Clear" batch for next round of submits
            batchData.count = 0;
        }

        currentCommandBuffer.end();

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    int StaticMeshRenderer::findExistingBatchIndex(ID_t identifier)
    {
        for (int  i = 0; i < _batches.size(); ++i)
        {
            BatchData& batchData = _batches[i];
            if (batchData.identifier == identifier && batchData.count + 1 <= s_maxBatchLength)
                return i;
        }
        return -1;
    }

    int StaticMeshRenderer::findFreeBatchIndex()
    {
        for (int  i = 0; i < _batches.size(); ++i)
        {
            BatchData& batchData = _batches[i];
            if (batchData.identifier == NULL_ID)
                return i;
        }
        return -1;
    }
}
