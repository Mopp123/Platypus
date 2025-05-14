#include "StaticMeshRenderer.h"
#include "platypus/graphics/Buffers.h"
#include "platypus/graphics/RenderCommand.h"
#include "platypus/core/Application.h"
#include "platypus/ecs/components/Transform.h"
#include "platypus/core/Debug.h"
#include <string>
#include <cstring>
#include <cmath>


namespace platypus
{
    size_t StaticMeshRenderer::s_maxBatches = 100;
    size_t StaticMeshRenderer::s_maxBatchLength = 10000;
    StaticMeshRenderer::StaticMeshRenderer(
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
        _vertexShader("TestVertexShader", ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT),
        _fragmentShader("TestFragmentShader", ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT),
        _materialDescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 4 } }
                },
                {
                    1,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
                    { { 5 } }
                }
            }
        )
    {
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
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM,
                true
            );
        }
    }

    StaticMeshRenderer::~StaticMeshRenderer()
    {
        for (BatchData& b : _batches)
            delete b.pInstancedBuffer;

        freeBatches();
        _materialDescriptorSetLayout.destroy();
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
            &_materialDescriptorSetLayout
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

    void StaticMeshRenderer::freeBatches()
    {
        for (BatchData& batch : _batches)
        {
            batch.identifier = NULL_ID;
            batch.count = 0;
        }
    }

    void StaticMeshRenderer::freeDescriptorSets()
    {
        std::unordered_map<ID_t, std::vector<DescriptorSet>>::iterator it;
        for (it = _descriptorSets.begin(); it != _descriptorSets.end(); ++it)
        {
            _descriptorPoolRef.freeDescriptorSets(it->second);
            it->second.clear();
        }
        _descriptorSets.clear();
    }

    void StaticMeshRenderer::submit(const Scene* pScene, entityID_t entity)
    {
        const StaticMeshRenderable* pRenderable = (const StaticMeshRenderable*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_STATIC_MESH_RENDERABLE
        );
        const Transform* pTransform = (const Transform*)pScene->getComponent(
            entity, ComponentType::COMPONENT_TYPE_TRANSFORM
        );
        const Matrix4f transformationMatrix = pTransform->globalMatrix;

        ID_t materialID = pRenderable->materialID;
        ID_t identifier = ID::hash(pRenderable->meshID, materialID);
        int foundBatchIndex = findExistingBatchIndex(identifier);
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
                    "@StaticMeshRenderer::submit "
                    "No free batches found!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            BatchData& batchData = _batches[freeBatchIndex];
            if (!occupyBatch(batchData, pRenderable->meshID, identifier))
            {
                Debug::log(
                    "@StaticMeshRenderer::submit "
                    "Failed to occupy batch!",
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return;
            }
            addToBatch(batchData, transformationMatrix);
        }
        // NOTE: Works only because using textures as identifiers and
        // only texture descriptor sets...
        if (!hasDescriptorSets(identifier))
            createDescriptorSets(identifier, materialID);
    }

    const CommandBuffer& StaticMeshRenderer::recordCommandBuffer(
        const RenderPass& renderPass,
        uint32_t viewportWidth,
        uint32_t viewportHeight,
        const Matrix4f& perspectiveProjectionMatrix,
        const Matrix4f& orthographicProjectionMatrix,
        const Matrix4f& viewMatrix,
        const DescriptorSet& dirLightDescriptorSet,
        size_t frame
    )
    {
        #ifdef PLATYPUS_DEBUG
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
        #endif

        CommandBuffer& currentCommandBuffer = _commandBuffers[_currentFrame];
        currentCommandBuffer.begin(renderPass);

        render::set_viewport(currentCommandBuffer, 0, 0, viewportWidth, viewportHeight, 0.0f, 1.0f);
        render::bind_pipeline(currentCommandBuffer, _pipeline);

        Matrix4f pushConstants[2] = { perspectiveProjectionMatrix, viewMatrix };
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

            // Make sure descriptor sets has been created
            if (_descriptorSets.find(batchData.identifier) == _descriptorSets.end())
            {
                Debug::log(
                    "@StaticMeshRenderer::recordCommandBuffer "
                    "No descriptor sets found for batch with identifier: " + std::to_string(batchData.identifier),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }

            batchData.pInstancedBuffer->updateDevice(
                batchData.pInstancedBuffer->accessData(),
                batchData.count * sizeof(Matrix4f),
                0
            );

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
                _descriptorSets[batchData.identifier][_currentFrame]
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
            // "Clear" the batch for next round of submits
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

    void StaticMeshRenderer::addToBatch(
        BatchData& batchData,
        const Matrix4f& transformationMatrix
    )
    {
        batchData.pInstancedBuffer->updateHost(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            batchData.count * sizeof(Matrix4f)
        );
        ++batchData.count;
    }

    bool StaticMeshRenderer::occupyBatch(
        BatchData& batchData,
        ID_t meshID,
        ID_t identifier
    )
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Mesh* pMesh = (const Mesh*)assetManager.getAsset(
            meshID,
            AssetType::ASSET_TYPE_MESH
        );
        #ifdef PLATYPUS_DEBUG
            if (!pMesh)
            {
                Debug::log(
                    "@StaticMeshRenderer::occupyBatch "
                    "No mesh found with ID: " + std::to_string(meshID),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                return false;
            }
        #endif

        batchData.identifier = identifier;
        batchData.pVertexBuffer = pMesh->getVertexBuffer();
        batchData.pIndexBuffer = pMesh->getIndexBuffer();

        return true;
    }

    bool StaticMeshRenderer::hasDescriptorSets(ID_t batchIdentifier) const
    {
        return _descriptorSets.find(batchIdentifier) != _descriptorSets.end();
    }

    void StaticMeshRenderer::createDescriptorSets(ID_t identifier, ID_t materialID)
    {
        Application* pApp = Application::get_instance();
        AssetManager& assetManager = pApp->getAssetManager();
        const Material* pMaterial = (const Material*)assetManager.getAsset(
            materialID,
            AssetType::ASSET_TYPE_MATERIAL
        );
        #ifdef PLATYPUS_DEBUG
            if (!pMaterial)
            {
                Debug::log(
                    "@StaticMeshRenderer::createDescriptorSets "
                    "No material found with ID: " + std::to_string(materialID),
                    Debug::MessageType::PLATYPUS_ERROR
                );
                PLATYPUS_ASSERT(false);
            }
        #endif

        const Texture* pDiffuseTexture = pMaterial->getDiffuseTexture();
        const Texture* pSpecularTexture = pMaterial->getSpecularTexture();

        size_t maxFramesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        for (int i = 0; i < maxFramesInFlight; ++i)
        {
            _descriptorSets[identifier].push_back(
                _descriptorPoolRef.createDescriptorSet(
                    &_materialDescriptorSetLayout,
                    {
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pDiffuseTexture },
                        { DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pSpecularTexture },
                    }
                )
            );
        }
        Debug::log("@StaticMeshRenderer::createDescriptorSets New descriptor sets created for batch with identifier: " + std::to_string(identifier));
    }

    void StaticMeshRenderer::freeBatchDescriptorSets(ID_t identifier)
    {
        std::unordered_map<ID_t, std::vector<DescriptorSet>>::iterator it = _descriptorSets.find(identifier);
        if (it != _descriptorSets.end())
            _descriptorPoolRef.freeDescriptorSets(it->second);
    }
}
