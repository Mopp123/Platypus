#include "GUIRenderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include <cstring>
#include <stdexcept>


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
        _masterRendererRef(masterRenderer),
        _commandPoolRef(commandPool),
        _descriptorPoolRef(descriptorPool),
        _requiredComponentsMask(requiredComponentsMask),
        _vertexShader("GUIVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _imgFragmentShader("GUIFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
        _fontFragmentShader("FontFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
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
        // Create common vertex and index buffers
        std::vector<float> vertexData = {
            0, 0,
            0, -1,
            1, -1,
            1, 0
        };
        std::vector<uint16_t> indices = {
            0, 1, 2,
            2, 3, 0
        };
        _pVertexBuffer = new Buffer(
            _commandPoolRef,
            vertexData.data(),
            sizeof(float) * 2,
            4,
            BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );
        _pIndexBuffer = new Buffer(
            _commandPoolRef,
            indices.data(),
            sizeof(uint16_t),
            indices.size(),
            BufferUsageFlagBits::BUFFER_USAGE_INDEX_BUFFER_BIT | BufferUsageFlagBits::BUFFER_USAGE_TRANSFER_DST_BIT,
            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STATIC,
            false
        );

        // Alloc batches
        _batches.resize(s_maxBatches);
        // Create empty instanced buffers for batches
        for (size_t i = 0; i < _batches.size(); ++i)
        {
            BatchData& batchData = _batches[i];
            std::vector<GUIRenderData> instanceBufferData(s_maxBatchLength);
            batchData.pInstancedBuffer = new Buffer(
                _commandPoolRef,
                instanceBufferData.data(),
                sizeof(GUITransform),
                instanceBufferData.size(),
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
        }
    }

    GUIRenderer::~GUIRenderer()
    {
        for (BatchData& b : _batches)
            delete b.pInstancedBuffer;

        freeBatches();
        _textureDescriptorSetLayout.destroy();

        delete _pVertexBuffer;
        delete _pIndexBuffer;
    }

    void GUIRenderer::allocCommandBuffers(uint32_t count)
    {
        _commandBuffers = _commandPoolRef.allocCommandBuffers(
            count,
            CommandBufferLevel::SECONDARY_COMMAND_BUFFER
        );
    }

    void GUIRenderer::freeCommandBuffers()
    {
        for (CommandBuffer& buffer : _commandBuffers)
            buffer.free();
        _commandBuffers.clear();
    }

    void GUIRenderer::createPipeline(
        const RenderPass& renderPass,
        float viewportWidth,
        float viewportHeight
    )
    {
        VertexBufferLayout vbLayout = {
            {
                { 0, ShaderDataType::Float2 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_VERTEX,
            0
        };
        VertexBufferLayout instancedVbLayout = {
            {
                { 1, ShaderDataType::Float4 },
                { 2, ShaderDataType::Float2 },
                { 3, ShaderDataType::Float4 }
            },
            VertexInputRate::VERTEX_INPUT_RATE_INSTANCE,
            1
        };
        std::vector<VertexBufferLayout> vertexBufferLayouts = { vbLayout, instancedVbLayout };
        std::vector<DescriptorSetLayout> descriptorSetLayouts = {
            _textureDescriptorSetLayout
        };

        Rect2D viewportScissor = { 0, 0, (uint32_t)viewportWidth, (uint32_t)viewportHeight };
        _imgPipeline.create(
            renderPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
            _vertexShader,
            _imgFragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_ALWAYS,
            true, // enable color blending
            sizeof(Matrix4f) + sizeof(float), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );

        _fontPipeline.create(
            renderPass,
            vertexBufferLayouts,
            descriptorSetLayouts,
            _vertexShader,
            _fontFragmentShader,
            viewportWidth,
            viewportHeight,
            viewportScissor,
            CullMode::CULL_MODE_BACK,
            FrontFace::FRONT_FACE_COUNTER_CLOCKWISE,
            true, // enable depth test
            DepthCompareOperation::COMPARE_OP_ALWAYS,
            true, // enable color blending
            sizeof(Matrix4f) + sizeof(float), // push constants size
            ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT // push constants' stage flags
        );
    }

    void GUIRenderer::destroyPipeline()
    {
        _imgPipeline.destroy();
        _fontPipeline.destroy();
    }

    void GUIRenderer::freeBatches()
    {
        for (BatchData& batch : _batches)
        {
            batch.type = BatchType::NONE;
            batch.textureID = NULL_ID;
            batch.count = 0;
        }
    }

    void GUIRenderer::freeDescriptorSets()
    {
        std::unordered_map<ID_t, std::vector<DescriptorSet>>::iterator it;
        for (it = _textureDescriptorSets.begin(); it != _textureDescriptorSets.end(); ++it)
        {
            _descriptorPoolRef.freeDescriptorSets(it->second);
            it->second.clear();
        }
        _textureDescriptorSets.clear();
    }

    void GUIRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        const GUIRenderable* pRenderable = (const GUIRenderable*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_GUI_RENDERABLE
        );
        const GUITransform* pTransform = (const GUITransform*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_GUI_TRANSFORM
        );

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        ID_t textureID = pRenderable->textureID;
        if (textureID == NULL_ID)
        {
            textureID = pAssetManager->getWhiteTexture()->getID();
        }

        int batchIndex = findExistingBatchIndex(pRenderable->layer, textureID);
        if (batchIndex == -1)
        {
            batchIndex = findFreeBatchIndex();
            if (batchIndex == -1)
            {
                Debug::log(
                    "@GUIRenderer::submit "
                    "No free batches found!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            if (!occupyBatch(
                    pRenderable->fontID != NULL_ID ? BatchType::TEXT : BatchType::IMAGE,
                    batchIndex,
                    pRenderable->layer, textureID
                ))
            {
                Debug::log(
                    "@GUIRenderer::submit "
                    "Failed to occupy batch!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
        }

        if (!hasDescriptorSets(textureID))
            createTextureDescriptorSets(textureID);

        if (pRenderable->fontID != NULL_ID)
            addToFontBatch(_batches[batchIndex], pRenderable, pTransform);
        else
            addToImageBatch(_batches[batchIndex], pRenderable, pTransform);
    }

    const CommandBuffer& GUIRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& orthographicProjectionMatrix,
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

        // The actual rendering stuff...
        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);

        std::unordered_map<uint32_t, std::vector<std::set<size_t>::iterator>> unusedBatches;

        std::map<uint32_t, std::set<size_t>>::iterator layerIt;
        for (layerIt = _toRender.begin(); layerIt != _toRender.end(); ++layerIt)
        {
            std::set<size_t>& layerBatches = layerIt->second;
            std::set<size_t>::iterator layerBatchIt;
            for (layerBatchIt = layerBatches.begin(); layerBatchIt != layerBatches.end(); ++layerBatchIt)
            {
                BatchData& batchData = _batches[*layerBatchIt];

                if (_batches[*layerBatchIt].count == 0)
                {
                    unusedBatches[layerIt->first].push_back(layerBatchIt);
                    continue;
                }

                // Make sure descriptor sets has been created
                if (_textureDescriptorSets.find(batchData.textureID) == _textureDescriptorSets.end())
                {
                    Debug::log(
                        "@GUIRenderer::recordCommandBuffer "
                        "No descriptor sets found for batch with textureID: " + std::to_string(batchData.textureID),
                        Debug::MessageType::PLATYPUS_ERROR
                    );
                    PLATYPUS_ASSERT(false);
                }

                // This should never actually happen?
                if (batchData.textureID == NULL_ID)
                    continue;

                batchData.pInstancedBuffer->updateDevice(
                    batchData.pInstancedBuffer->accessData(),
                    batchData.count * sizeof(GUIRenderData),
                    0
                );

                render::bind_pipeline(
                    currentCommandBuffer,
                    batchData.type == BatchType::IMAGE ? _imgPipeline : _fontPipeline
                );

                float pushConstantsData[20];
                memcpy(pushConstantsData, orthographicProjectionMatrix.getRawArray(), sizeof(Matrix4f));
                memcpy(((PE_byte*)pushConstantsData) + sizeof(Matrix4f), &batchData.textureAtlasRows, sizeof(float));
                render::push_constants(
                    currentCommandBuffer,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    0,
                    sizeof(Matrix4f) + sizeof(float),
                    pushConstantsData,
                    {
                        { 0, ShaderDataType::Mat4 },
                        { 1, ShaderDataType::Float }
                    }
                );

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
                    { _textureDescriptorSets[batchData.textureID][_currentFrame] },
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

        // Erase unused batches
        std::unordered_map<uint32_t, std::vector<std::set<size_t>::iterator>>::iterator eraseLayerIt;
        for (eraseLayerIt = unusedBatches.begin(); eraseLayerIt != unusedBatches.end(); ++eraseLayerIt)
        {
            std::vector<std::set<size_t>::iterator>& layerBatches = eraseLayerIt->second;
            std::vector<std::set<size_t>::iterator>::iterator eraseBatchIt;
            for (eraseBatchIt = layerBatches.begin(); eraseBatchIt != layerBatches.end(); ++eraseBatchIt)
            {
                _toRender[eraseLayerIt->first].erase(*eraseBatchIt);
                Debug::log("___TEST___ERASED UNUSED BATCH!");
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

    void GUIRenderer::addToImageBatch(
        BatchData& batchData,
        const GUIRenderable* pRenderable,
        const GUITransform* pTransform
    )
    {
        GUIRenderData renderData = {
            {
                pTransform->position.x,
                pTransform->position.y,
                pTransform->scale.x,
                pTransform->scale.y
            },
            pRenderable->textureOffset,
            pRenderable->color
        };
        batchData.pInstancedBuffer->updateHost(
            (void*)(&renderData),
            sizeof(GUIRenderData),
            batchData.count * sizeof(GUIRenderData)
        );
        ++batchData.count;
    }

    void GUIRenderer::addToFontBatch(
        BatchData& batchData,
        const GUIRenderable* pRenderable,
        const GUITransform* pTransform
    )
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        const Font* pFont = (const Font*)pAssetManager->getAsset(pRenderable->fontID, AssetType::ASSET_TYPE_FONT);
        const std::unordered_map<wchar_t, FontGlyphData>& glyphMapping = pFont->getGlyphMapping();

        const float originalX = pTransform->position.x;
        float posX = originalX;
        float posY = pTransform->position.y;

        // TODO: Support text scaling
        float scaleFactorX = 1.0f;
        float scaleFactorY = 1.0f;

        float charWidth = pFont->getTilePixelWidth() * scaleFactorX;
        float charHeight = pFont->getMaxCharHeight() * scaleFactorY;

        for (wchar_t c : pRenderable->text)
        {
            // Check, do we want to change line?
            if (c == '\n')
            {
                posY += charHeight;
                posX = originalX;
                continue;
            }
            // Empty space uses font specific details so no any special cases here..
            else if (c == ' ')
            {
            }
            // Make sure not draw nulldtm chars accidentally..
            else if (c == 0)
            {
                break;
            }

            try
            {
                const FontGlyphData& glyphData = glyphMapping.at(c);

                float x = (posX + (float)glyphData.bearingX);
                // - ch because we want origin to be at 0,0 but we also need to add the bearingY so.. gets fucked without this..
                float y = posY - (float)glyphData.bearingY + charHeight;

                GUIRenderData renderData = {
                    {
                        (float)(int)x, (float)(int)y,
                        (float)(int)charWidth, (float)(int)charHeight,
                    },
                    { (float)glyphData.textureOffsetX, (float)glyphData.textureOffsetY },
                    pRenderable->color
                };
                batchData.pInstancedBuffer->updateHost(
                    (void*)(&renderData),
                    sizeof(GUIRenderData),
                    batchData.count * sizeof(GUIRenderData)
                );
                ++batchData.count;

                // NOTE: Don't quite understand this.. For some reason on some fonts >> 7 works better...
                posX += ((float)(glyphData.advance >> 6)) * scaleFactorX; // now advance cursors for next glyph (note that advance is number of 1/64 pixels). bitshift by 6 to get value in pixels (2^6 = 64) | OLD COMMENT: 2^5 = 32 (pixel size of the font..)
            }
            catch (const std::out_of_range& e)
            {
                Debug::log(
                    "@GUIRenderer::addToFontBatch "
                    "No glyph data found for character: " + std::string(c, 1) + " (value: " + std::to_string(c) + ")",
                    Debug::MessageType::PLATYPUS_ERROR
                );
            }
        }
    }

    bool GUIRenderer::occupyBatch(
        BatchType batchType,
        size_t batchIndex,
        uint32_t layer,
        ID_t textureID
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        const Texture* pTexture = (const Texture*)pAssetManager->getAsset(
            textureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
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
        batchData.type = batchType;
        batchData.textureID = textureID;
        batchData.textureAtlasRows = (float)pTexture->getAtlasRowCount();
        _toRender[layer].insert(batchIndex);
        return true;
    }

    bool GUIRenderer::hasDescriptorSets(ID_t batchIdentifier) const
    {
        return _textureDescriptorSets.find(batchIdentifier) != _textureDescriptorSets.end();
    }

    void GUIRenderer::createTextureDescriptorSets(ID_t textureID)
    {
        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        const Texture* pTexture = (const Texture*)pAssetManager->getAsset(
            textureID,
            AssetType::ASSET_TYPE_TEXTURE
        );
        #ifdef PLATYPUS_DEBUG
            if (!pTexture)
            {
                Debug::log(
                    "@GUIRenderer::createDescriptorSets "
                    "No texture found with ID: " + std::to_string(textureID),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        for (int i = 0; i < maxFramesInFlight; ++i)
        {
            _textureDescriptorSets[textureID].push_back(
                _descriptorPoolRef.createDescriptorSet(
                    &_textureDescriptorSetLayout,
                    { { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pTexture } }
                )
            );
        }
        Debug::log("@GUIRenderer::createDescriptorSets New texture descriptor sets created for batch with textureID: " + std::to_string(textureID));
    }

    void GUIRenderer::freeTextureDescriptorSets(ID_t textureID)
    {
        std::unordered_map<ID_t, std::vector<DescriptorSet>>::iterator it = _textureDescriptorSets.find(textureID);
        if (it != _textureDescriptorSets.end())
            _descriptorPoolRef.freeDescriptorSets(it->second);
    }
}
