#include "TestRenderer.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cmath>


namespace platypus
{
    size_t TestRenderer::s_maxBatches = 20;
    size_t TestRenderer::s_maxBatchLength = 200;

    static size_t s_TEST_MAX_ENTITIES = 1000;
    TestRenderer::TestRenderer(
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
        _testDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { 2, ShaderDataType::Mat4 } }
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
                    { { 5 } }
                }
            }
        )
    {
        _batches.resize(s_maxBatches);
        for (size_t i = 0; i < _batches.size(); ++i)
        {
            _freeBatchIndices.insert(i);
            BatchData& batchData = _batches[i];
            size_t uniformBufferElemSize = get_dynamic_uniform_buffer_element_size(sizeof(Matrix4f));
            std::vector<Matrix4f> transformsBuffer(s_maxBatchLength);

            for (int i = 0; i < swapchain.getMaxFramesInFlight(); ++i)
            {
                Buffer* pUniformBuffer = new Buffer(
                    _commandPoolRef,
                    transformsBuffer.data(),
                    uniformBufferElemSize,
                    transformsBuffer.size(),
                    BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
                );
                batchData.transformsBuffer.push_back(pUniformBuffer);

                batchData.transformsDescriptorSets.push_back(
                    descriptorPool.createDescriptorSet(
                        &_testDescriptorSetLayout,
                        { pUniformBuffer }
                    )
                );
            }
            batchData.transformsBufferOffsets.resize(s_maxBatchLength);
            for (int i = 0; i < s_maxBatchLength; ++i)
            {
                batchData.transformsBufferOffsets[i] = i * uniformBufferElemSize;
            }
        }

        // Create object uniform buffers and descriptor sets
        /*
        size_t objUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(sizeof(Matrix4f));
        std::vector<Matrix4f> objUniformBufferData(s_TEST_MAX_ENTITIES);
        for (int i = 0; i < swapchain.getMaxFramesInFlight(); ++i)
        {
            Buffer* pObjUniformBuffer = new Buffer(
                _commandPoolRef,
                objUniformBufferData.data(),
                objUniformBufferElementSize,
                objUniformBufferData.size(),
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC
            );
            _testUniformBuffer.push_back(pObjUniformBuffer);

            _testDescriptorSets.push_back(
                descriptorPool.createDescriptorSet(
                    &_testDescriptorSetLayout,
                    { pObjUniformBuffer }
                )
            );
        }
        */
    }

    TestRenderer::~TestRenderer()
    {
        /*
        for (Buffer* pUniformBuffer : _testUniformBuffer)
            delete pUniformBuffer;
        _testUniformBuffer.clear();

        */

        for (BatchData& b : _batches)
        {
            for (Buffer* pBuffer : b.transformsBuffer)
                delete pBuffer;
            // NOTE: shouldn't we also delete descriptor sets?
        }

        _testDescriptorSetLayout.destroy();
        _textureDescriptorSetLayout.destroy();
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
        std::vector<VertexBufferLayout> vertexBufferLayouts = { vbLayout };
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = {
            &_testDescriptorSetLayout,
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

    void TestRenderer::destroyPipeline()
    {
        _pipeline.destroy();
    }

    // NOTE: This can't create new batch if a batch exists for this texture but it's full
    //  -> should create new batch using that same texture
    void TestRenderer::submit(
        const StaticMeshRenderable* pRenderable,
        const Matrix4f& transformationMatrix
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Mesh* pMesh = assetManager.getMesh(pRenderable->meshID);

        ID_t textureID = pRenderable->textureID;
        if (_occupiedBatches.find(textureID) != _occupiedBatches.end())
        {
            // add to existing batch
            BatchData& currentBatch = _batches[_occupiedBatches[textureID]];
            if (currentBatch.count + 1 >= s_maxBatchLength)
            {
                Debug::log(
                    "@TestRenderer::submit "
                    "Existing batch was found but it was full!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            currentBatch.transformsBuffer[_currentFrame]->update(
                (void*)(&transformationMatrix),
                sizeof(Matrix4f),
                currentBatch.count * currentBatch.transformsBuffer[_currentFrame]->getDataElemSize()
            );
            ++currentBatch.count;
        }
        else
        {
            // occupy new batch
            if (_freeBatchIndices.empty())
            {
                Debug::log(
                    "@TestRenderer::submit "
                    "Submit requires new batch but no free batches were found!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            size_t freeBatchIndex = *_freeBatchIndices.begin();
            BatchData& currentBatch = _batches[freeBatchIndex];
            _occupiedBatches[textureID] = freeBatchIndex;
            _freeBatchIndices.erase(freeBatchIndex);

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

            currentBatch.transformsBuffer[_currentFrame]->update(
                (void*)(&transformationMatrix),
                sizeof(Matrix4f),
                currentBatch.count * currentBatch.transformsBuffer[_currentFrame]->getDataElemSize()
            );
            ++currentBatch.count;
        }
        // Update transforms uniform buffer
    }
    /*
    void TestRenderer::submit(
        const StaticMeshRenderable* pRenderable,
        const Matrix4f& transformationMatrix
    )
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
            size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
            for (int i = 0; i < maxFramesInFlight; ++i)
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
    */

    const CommandBuffer& TestRenderer::recordCommandBuffer(
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
                "@TestRenderer::recordCommandBuffer "
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

        std::unordered_map<ID_t, size_t>::const_iterator batchIt;
        for (batchIt = _occupiedBatches.begin(); batchIt != _occupiedBatches.end(); ++batchIt)
        {
            BatchData& batchData = _batches[batchIt->second];

            render::bind_vertex_buffers(currentCommandBuffer, { batchData.pVertexBuffer });
            render::bind_index_buffer(currentCommandBuffer, batchData.pIndexBuffer);

            std::vector<DescriptorSet> descriptorSetsToBind = {
                batchData.transformsDescriptorSets[_currentFrame],
                dirLightDescriptorSet,
                batchData.textureDescriptorSets[_currentFrame]
            };

            for (int i = 0; i < batchData.count; ++i)
            {
                render::bind_descriptor_sets(
                    currentCommandBuffer,
                    descriptorSetsToBind,
                    { batchData.transformsBufferOffsets[i] }
                );

                render::draw_indexed(
                    currentCommandBuffer,
                    (uint32_t)batchData.pIndexBuffer->getDataLength(),
                    1
                );
            }
            batchData.count = 0;
        }

        /*
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
            std::vector<DescriptorSet> descriptorSetsToBind = {
                _testDescriptorSets[frame],
                dirLightDescriptorSet,
                renderData.descriptorSets[frame]
            };
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
        */

        currentCommandBuffer.end();

        // NOTE: Only temporarely clearing this here every frame!
        //_renderList.clear();



        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }
}
