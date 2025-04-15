#include "GUIRenderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"


namespace platypus
{
    size_t GUIRenderer::s_maxBatches = 100;
    size_t GUIRenderer::s_maxBatchLength = 1000;
    GUIRenderer::GUIRenderer(
        const MasterRenderer& masterRenderer,
        CommandPool& commandPool,
        DescriptorPool& descriptorPool,
        uint64_t requiredComponentsMask
    ) :
        Renderer(
            masterRenderer,
            commandPool,
            descriptorPool,
            requiredComponentsMask
        ),
        _vertexShader("GUIVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("GUIFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
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
        // Create common vertex and index buffers
        std::vector<float> vertexData = {
            -1,  1,     0, 1,
            -1, -1,     0, 0,
             1, -1,     1, 0,
             1,  1,     1, 1
        };
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };
        _pVertexBuffer = new Buffer(
            _commandPoolRef,
            vertexData.data(),
            sizeof(float) * 4,
            4,
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );
        _pIndexBuffer = new Buffer(
            _commandPoolRef,
            indices.data(),
            sizeof(uint16_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC
        );

        // Alloc batches
        _batches.resize(s_maxBatches);
        // Create empty instanced buffers for batches
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

    GUIRenderer::~GUIRenderer()
    {
        for (BatchData& b : _batches)
        {
            delete b.pInstancedBuffer;
            // NOTE: shouldn't we also delete descriptor sets?
        }
        _textureDescriptorSetLayout.destroy();

        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    void GUIRenderer::createPipeline(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight,
        const DescriptorSetLayout& dirLightDescriptorSetLayout
    )
    {
        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float3 },
                { 1, ShaderDataType::Float2 },
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
        std::vector<const DescriptorSetLayout*> descriptorSetLayouts = { &_textureDescriptorSetLayout };

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
            DepthCompareOperation::COMPARE_OP_LESS,
            false, // enable color blending
            sizeof(Matrix4f), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void GUIRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        const GUIRenderable* pRenderable = (const GUIRenderable*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
        );
        const Transform* pTransform = (const Transform*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        const Matrix4f transformationMatrix = pTransform->globalMatrix;

        AssetManager& assetManager = Application::get_instance()->getAssetManager();
        ID_t textureID = pRenderable->textureID;

        int foundBatchIndex = findExistingBatchIndex(pRenderable->layer, textureID);
        if (foundBatchIndex != -1)
        {
            addToBatch(_batches[foundBatchIndex], transformationMatrix);
        }
        else
        {
            int freeBatchIndex = findFreeBatchIndex();
            if (freeBatchIndex == -1)
            {
                Debug::log(
                    "@GUIRenderer::submit "
                    "No free batches found!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            BatchData& batchData = _batches[freeBatchIndex];
            if (!occupyBatch(freeBatchIndex, pRenderable->layer, textureID))
            {
                Debug::log(
                    "@GUIRenderer::submit "
                    "Failed to occupy batch!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            addToBatch(batchData, transformationMatrix);
        }
    }

    const CommandBuffer& GUIRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& projectionMatrix,
        const Matrix4f& viewMatrix,
        const DescriptorSet& dirLightDescriptorSet,
        size_t frame
    )
    {
        #ifdef PLATYPUS_DEBUG
            if (_currentFrame >= _commandBuffers.size())
            {
                Debug::log(
                    "@GUIRenderer::recordCommandBuffer "
                    "Frame index(" + std::to_string(_currentFrame) + ") out of bounds! "
                    "Allocated command buffer count is " + std::to_string(_commandBuffers.size()),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::bind_pipeline(currentCommandBuffer, _pipeline);

        Matrix4f pushConstants[1] = { projectionMatrix };
        render::push_constants(
            currentCommandBuffer,
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(Matrix4f),
            pushConstants,
            {
                { 0, ShaderDataType::Mat4 }
            }
        );


        std::map<uint32_t, std::set<size_t>>::const_iterator layerIt;
        for (layerIt = _toRender.begin(); layerIt != _toRender.end(); ++layerIt)
        {
            const std::set<size_t>& layerBatches = layerIt->second;
            std::set<size_t>::const_iterator layerBatchIt;
            for (layerBatchIt = layerBatches.begin(); layerBatchIt != layerBatches.end(); ++layerBatchIt)
            {
                BatchData& batchData = _batches[*layerBatchIt];

                // This should never actually happen?
                if (batchData.textureID == NULL_ID)
                    continue;

                render::bind_vertex_buffers(
                    currentCommandBuffer,
                    {
                        _pVertexBuffer,
                        batchData.pInstancedBuffer
                    }
                );
                render::bind_index_buffer(currentCommandBuffer, _pIndexBuffer);

                render::bind_descriptor_sets(
                    currentCommandBuffer,
                    { batchData.textureDescriptorSets[_currentFrame] },
                    { }
                );

                render::draw_indexed(
                    currentCommandBuffer,
                    (uint32_t)_pIndexBuffer->getDataLength(),
                    batchData.count
                );
                // "Clear" the batch for next round of submits
                // NOTE: Might be issue here if shitload of gui stuff
                //  -> occupied batches gets never really cleared!
                //  TODO: Truly clear batches at least on scene switch
                //      + maybe some clever way to deal with that withing the same scene too...
                batchData.count = 0;
            }
        }

        currentCommandBuffer.end();

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        _currentFrame = (_currentFrame + 1) % maxFramesInFlight;

        return currentCommandBuffer;
    }

    int GUIRenderer::findExistingBatchIndex(uint32_t layer, ID_t textureID)
    {
        std::map<uint32_t, std::set<size_t>>::iterator layerIt = _toRender.find(layer);
        if (layerIt == _toRender.end())
            return -1;

        const std::set<size_t>& layerBatches = layerIt->second;
        std::set<size_t>::const_iterator layerBatchIt;
        for (layerBatchIt = layerBatches.begin(); layerBatchIt != layerBatches.end(); ++layerBatchIt)
        {
            size_t batchIndex = *layerBatchIt;
            BatchData& batchData = _batches[batchIndex];
            if (batchData.textureID == textureID && batchData.count + 1 <= s_maxBatchLength)
                return batchIndex;
        }
        return -1;
    }

    int GUIRenderer::findFreeBatchIndex()
    {
        for (int  i = 0; i < _batches.size(); ++i)
        {
            if (_batches[i].textureID == NULL_ID)
                return i;
        }
        return -1;
    }

    void GUIRenderer::addToBatch(
        BatchData& batchData,
        const Matrix4f& transformationMatrix
    )
    {
        batchData.pInstancedBuffer->update(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            batchData.count * sizeof(Matrix4f)
        );
        ++batchData.count;
    }

    bool GUIRenderer::occupyBatch(
        size_t batchIndex,
        uint32_t layer,
        ID_t textureID
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Texture* pTexture = assetManager.getTexture(textureID);
        #ifdef PLATYPUS_DEBUG
            if (!pTexture)
            {
                Debug::log(
                    "@GUIRenderer::occupyBatch "
                    "No texture found with ID: " + std::to_string(textureID),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return false;
            }
        #endif

        BatchData& batchData = _batches[batchIndex];
        batchData.textureID = textureID;
        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        for (int i = 0; i < maxFramesInFlight; ++i)
        {
            batchData.textureDescriptorSets.push_back(
                _descriptorPoolRef.createDescriptorSet(
                    &_textureDescriptorSetLayout,
                    { pTexture }
                )
            );
        }
        _toRender[layer].insert(batchIndex);
        return true;
    }
}
