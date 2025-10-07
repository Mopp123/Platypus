#include "Batch.hpp"
#include "platypus/assets/TerrainMesh.hpp"
#include "platypus/core/Application.h"
#include "platypus/assets/AssetManager.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    DescriptorSetLayout Batcher::s_jointDescriptorSetLayout;
    DescriptorSetLayout Batcher::s_terrainDescriptorSetLayout;

    Batcher::Batcher(
        MasterRenderer& masterRenderer,
        DescriptorPool& descriptorPool,
        size_t maxStaticBatchLength,
        size_t maxSkinnedBatchLength,
        size_t maxTerrainBatchLength,
        size_t maxSkinnedMeshJoints
    ) :
        _masterRendererRef(masterRenderer),
        _descriptorPoolRef(descriptorPool),
        _maxStaticBatchLength(maxStaticBatchLength),
        _maxSkinnedBatchLength(maxSkinnedBatchLength),
        _maxTerrainBatchLength(maxTerrainBatchLength),
        _maxSkinnedMeshJoints(maxSkinnedMeshJoints)
    {
        s_jointDescriptorSetLayout = DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { ShaderDataType::Mat4, (int)_maxSkinnedMeshJoints } }
                }
            }
        );

        s_terrainDescriptorSetLayout = DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER,
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    {
                        { ShaderDataType::Mat4 },
                        { ShaderDataType::Float2 },
                    }
                }
            }
        );
    }

    Batcher::~Batcher()
    {
        s_jointDescriptorSetLayout.destroy();
        s_terrainDescriptorSetLayout.destroy();
    }

    ID_t Batcher::getBatchID(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);

        std::unordered_map<ID_t, size_t>::const_iterator indexIt = _identifierBatchMapping.find(identifier);
        if (indexIt != _identifierBatchMapping.end())
            return indexIt->first;

        return NULL_ID;
    }

    // NOTE: Could even create the pipelines dynamically here, so wouldn't require having those inside
    // Material anymore?
    ID_t Batcher::createStaticBatch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier))
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pMaterialPipeline = nullptr;
        Pipeline* pMaterialShadowPipeline = nullptr;
        if (pMaterial->getPipelineData() == nullptr)
        {
            pMaterial->createPipeline(
                _masterRendererRef.getSwapchain().getRenderPassPtr(),
                pMesh->getVertexBufferLayout(),
                true, // instanced
                false, // skinned
                false // shadow pipeline
            );
        }

        if (pMaterial->getShadowPipelineData() == nullptr)
        {
            pMaterial->createPipeline(
                &_masterRendererRef.getTestRenderPass(),
                pMesh->getVertexBufferLayout(),
                true, // instanced
                false, // skinned
                true // shadow pipeline
            );
        }

        pMaterialPipeline = pMaterial->getPipelineData()->pPipeline;
        pMaterialShadowPipeline = pMaterial->getShadowPipelineData()->pPipeline;

        // Create the instanced transforms buffer
        const size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        std::vector<Buffer*> transformBuffers(framesInFlight);
        createBatchInstancedBuffers(
            identifier,
            sizeof(Matrix4f),
            _maxStaticBatchLength,
            framesInFlight,
            transformBuffers
        );

        std::vector<BatchShaderResource> shaderResources(1);
        createBatchShaderResources(
            framesInFlight,
            identifier,
            1,
            {
                {
                    ShaderResourceType::MATERIAL,
                    sizeof(Vector4f),
                    pMaterial->getDescriptorSetLayout(),
                    pMaterial->getTextures()
                }
            },
            shaderResources
        );

        // TODO: Some better way to deal with this?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            shaderResources[0].descriptorSet.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                shaderResources[0].descriptorSet
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::STATIC_INSTANCED,
            pMaterialPipeline,
            pMaterialShadowPipeline,
            useDescriptorSets,
            0, // dynamic uniform buffer element size
            { pMesh->getVertexBuffer() },
            { transformBuffers },
            pMesh->getIndexBuffer(),
            0, // push constants size
            { }, // push constant uniform infos
            nullptr, // push constants data
            ShaderStageFlagBits::SHADER_STAGE_NONE, // push constants shader stage flags
            1, // repeat count
            0, // instance count
            materialID // materialAssetID
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);
        return identifier;
    }

    ID_t Batcher::createSkinnedBatch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier))
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pMaterialSkinnedPipeline = nullptr;
        if (pMaterial->getSkinnedPipelineData() == nullptr)
        {
            pMaterial->createPipeline(
                _masterRendererRef.getSwapchain().getRenderPassPtr(),
                pMesh->getVertexBufferLayout(),
                false, // instanced
                true, // skinned
                false // shadow pipeline
            );
        }

        pMaterialSkinnedPipeline = pMaterial->getSkinnedPipelineData()->pPipeline;

        // Create joint uniform buffer and descriptor sets
        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        size_t dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * _maxSkinnedMeshJoints
        );

        std::vector<BatchShaderResource> shaderResources(2);
        createBatchShaderResources(
            framesInFlight,
            identifier,
            _maxSkinnedBatchLength,
            {
                {
                    ShaderResourceType::ANY,
                    dynamicUniformBufferElementSize,
                    s_jointDescriptorSetLayout,
                    { }
                },
                {
                    ShaderResourceType::MATERIAL,
                    sizeof(Vector4f),
                    pMaterial->getDescriptorSetLayout(),
                    pMaterial->getTextures()
                }
            },
            shaderResources
        );

        // TODO: Some better way to deal with this?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            shaderResources[0].descriptorSet.size(),
            shaderResources[1].descriptorSet.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                shaderResources[0].descriptorSet,
                shaderResources[1].descriptorSet
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::SKINNED,
            pMaterialSkinnedPipeline,
            nullptr,
            useDescriptorSets,
            (uint32_t)dynamicUniformBufferElementSize, // dynamic uniform buffer element size
            { pMesh->getVertexBuffer() },
            { }, // dynamic/instanced vertex buffers
            pMesh->getIndexBuffer(),
            0, // push constants size
            { }, // push constant uniform infos
            nullptr, // push constants data
            ShaderStageFlagBits::SHADER_STAGE_NONE, // push constants shader stage flags
            0, // repeat count
            0, // instance count
            materialID // materialAssetID
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);
        return identifier;
    }

    ID_t Batcher::createTerrainBatch(
        ID_t terrainMeshID,
        ID_t materialID,
        const RenderPass& offscreenRenderPass
    )
    {
        ID_t identifier = ID::hash(terrainMeshID, materialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier))
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        TerrainMesh* pMesh = (TerrainMesh*)pAssetManager->getAsset(terrainMeshID, AssetType::ASSET_TYPE_TERRAIN_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pMaterialPipeline = nullptr;
        Pipeline* pMaterialShadowPipeline = nullptr;
        if (pMaterial->getPipelineData() == nullptr)
        {
            pMaterial->createPipeline(
                _masterRendererRef.getSwapchain().getRenderPassPtr(),
                pMesh->getVertexBufferLayout(),
                false, // instanced
                false, // skinned
                false // shadow pipeline
            );
        }

        //if (pMaterial->getShadowPipelineData() == nullptr)
        //{
        //    pMaterial->createPipeline(
        //        &_masterRendererRef.getTestRenderPass(),
        //        pMesh->getVertexBufferLayout(),
        //        false, // instanced
        //        false, // skinned
        //        true // shadow pipeline
        //    );
        //}

        pMaterialPipeline = pMaterial->getPipelineData()->pPipeline;
        //pMaterialShadowPipeline = pMaterial->getShadowPipelineData()->pPipeline;

        // Create uniform buffers holding transformation matrix, tile size
        // and vertices. Also create descriptor sets for these
        // TODO: Some place to store the element size of the buffer
        //  -> might become issue when the elem size changes!
        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        size_t dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) + sizeof(Vector2f)
        );

        std::vector<BatchShaderResource> shaderResources(2);
        createBatchShaderResources(
            framesInFlight,
            identifier,
            _maxTerrainBatchLength,
            {
                {
                    ShaderResourceType::ANY,
                    dynamicUniformBufferElementSize,
                    s_terrainDescriptorSetLayout,
                    { }
                },
                {
                    ShaderResourceType::MATERIAL,
                    sizeof(Vector4f),
                    pMaterial->getDescriptorSetLayout(),
                    pMaterial->getTextures()
                }
            },
            shaderResources
        );

        // TODO: Some better way to deal with these?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            shaderResources[0].descriptorSet.size(),
            shaderResources[1].descriptorSet.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                shaderResources[0].descriptorSet,
                shaderResources[1].descriptorSet
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::TERRAIN,
            pMaterialPipeline,
            nullptr,//pMaterialShadowPipeline,
            useDescriptorSets,
            (uint32_t)dynamicUniformBufferElementSize, // dynamic uniform buffer element size
            { pMesh->getVertexBuffer() },
            { }, // dynamic/instanced vertex buffers
            pMesh->getIndexBuffer(),
            0, // push constants size
            { }, // push constant uniform infos
            nullptr, // push constants data
            ShaderStageFlagBits::SHADER_STAGE_NONE, // push constants shader stage flags
            0, // repeat count
            0, // instance count,
            materialID // materialAssetID
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);
        return identifier;
    }

    void Batcher::addToStaticBatch(
        ID_t identifier,
        const Matrix4f& transformationMatrix,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);
        if (!pBatch)
            return;

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of thing where multiple batches can exist for the same identifier
        if (pBatch->instanceCount >= _maxStaticBatchLength)
        {
            Debug::log(
                "@Batcher::addToStaticBatch "
                "Batch is full. Maximum static batch length is " + std::to_string(_maxStaticBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Buffer* pInstancedBuffer = getBatchBuffer(identifier, 0, currentFrame);
        pInstancedBuffer->updateHost(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            sizeof(Matrix4f) * pBatch->instanceCount
        );
        ++pBatch->instanceCount;
        pBatch->repeatCount = 1;
    }

    void Batcher::addToSkinnedBatch(
        ID_t identifier,
        void* pJointData,
        size_t jointDataSize,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->repeatCount >= _maxSkinnedBatchLength)
        {
            Debug::log(
                "@Batcher::addToSkinnedBatch "
                "Batch is full. Maximum skinned batch length is " + std::to_string(_maxSkinnedBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Buffer* pJointBuffer = getBatchBuffer(identifier, 0, currentFrame);
        uint32_t jointBufferOffset = pBatch->repeatCount * pJointBuffer->getDataElemSize();
        pJointBuffer->updateHost(
            pJointData,
            jointDataSize,
            jointBufferOffset
        );
        pBatch->instanceCount = 1;
        ++pBatch->repeatCount;
    }

    void Batcher::addToTerrainBatch(
        ID_t identifier,
        const Matrix4f& transformationMatrix,
        float tileSize,
        uint32_t verticesPerRow,
        size_t currentFrame
    )
    {
        Batch* pBatch = getBatch(identifier);

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->repeatCount >= _maxTerrainBatchLength)
        {
            Debug::log(
                "@Batcher::addToTerrainBatch "
                "Batch is full. Maximum skinned batch length is " + std::to_string(_maxSkinnedBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        const size_t terrainDataSize = sizeof(Matrix4f) + sizeof(Vector2f);
        std::vector<PE_byte> terrainData(terrainDataSize);
        memcpy(terrainData.data(), &transformationMatrix, sizeof(Matrix4f));
        memcpy(terrainData.data() + sizeof(Matrix4f), &tileSize, sizeof(float));
        float fVerticesPerRow = verticesPerRow;
        memcpy(terrainData.data() + sizeof(Matrix4f) + sizeof(float), &fVerticesPerRow, sizeof(float));

        Buffer* pTerrainBuffer = getBatchBuffer(identifier, 0, currentFrame);
        uint32_t terrainBufferOffset = pBatch->repeatCount * pTerrainBuffer->getDataElemSize();
        pTerrainBuffer->updateHost(
            terrainData.data(),
            terrainDataSize,
            terrainBufferOffset
        );

        pBatch->instanceCount = 1;
        ++pBatch->repeatCount;
    }

    void Batcher::updateDeviceSideBuffers(size_t currentFrame)
    {
        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        std::unordered_map<ID_t, size_t>::const_iterator it;
        for (it = _batchShaderResourceMapping.begin(); it != _batchShaderResourceMapping.end(); ++it)
        {
            // TODO: Make safer!
            const ID_t batchID = it->first;
            const Batch* pBatch = _batches[_identifierBatchMapping[batchID]];
            if (pBatch->instanceCount > 0 || pBatch->repeatCount > 0)
            {
                // TODO: Make safer!
                const size_t resourceIndex = _batchShaderResourceMapping[batchID];
                for (BatchShaderResource& shaderResource : _allocatedShaderResources[resourceIndex])
                {
                    Buffer* pBuffer = shaderResource.buffer[currentFrame];
                    // Update material properties if resource was marked as Material
                    if (shaderResource.type == ShaderResourceType::MATERIAL && pBatch->materialAssetID != NULL_ID)
                    {
                        Vector4f materialProperties(0, 0, 0, 0);
                        Material* pMaterial = (Material*)pAssetManager->getAsset(
                            pBatch->materialAssetID,
                            AssetType::ASSET_TYPE_MATERIAL
                        );
                        materialProperties.x = pMaterial->getSpecularStrength();
                        materialProperties.y = pMaterial->getShininess();
                        materialProperties.z = pMaterial->isShadeless();

                        pBuffer->updateDeviceAndHost(
                            &materialProperties,
                            sizeof(Vector4f),
                            0
                        );
                    }
                    else
                    {
                        pBuffer->updateDevice(
                            pBuffer->accessData(),
                            pBuffer->getTotalSize(),
                            0
                        );
                    }
                }
            }
        }
    }

    void Batcher::resetForNextFrame()
    {
        for (Batch* pBatch : _batches)
        {
            pBatch->repeatCount = 0;
            pBatch->instanceCount = 0;
        }
    }

    void Batcher::freeBatches()
    {
        // Free batches themselves
        for (Batch* pBatch : _batches)
            delete pBatch;

        _batches.clear();
        _identifierBatchMapping.clear();

        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();
        for (std::vector<BatchShaderResource>& batchResources : _allocatedShaderResources)
        {
            for (BatchShaderResource& resource : batchResources)
            {
                for (Buffer* pBuffer : resource.buffer)
                    delete pBuffer;

                descriptorPool.freeDescriptorSets(resource.descriptorSet);
            }
        }
        _allocatedShaderResources.clear();
        _batchShaderResourceMapping.clear();
    }

    const std::vector<Batch*>& Batcher::getBatches() const
    {
        return _batches;
    }

    const DescriptorSetLayout& Batcher::get_joint_descriptor_set_layout()
    {
        return s_jointDescriptorSetLayout;
    }

    const DescriptorSetLayout& Batcher::get_terrain_descriptor_set_layout()
    {
        return s_terrainDescriptorSetLayout;
    }

    void Batcher::createBatchInstancedBuffers(
        ID_t batchID,
        size_t bufferElementSize,
        size_t maxBatchLength,
        size_t framesInFlight,
        std::vector<Buffer*>& outBuffers
    )
    {
        std::vector<char> bufferData(bufferElementSize * maxBatchLength);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            outBuffers[i] = new Buffer(
                bufferData.data(),
                bufferElementSize,
                maxBatchLength,
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM,
                true
            );
        }
        addToAllocatedShaderResources(batchID, outBuffers, {});
    }

    void Batcher::createBatchShaderResources(
        size_t framesInFlight,
        ID_t batchID,
        size_t maxBatchLength,
        const std::vector<ShaderResourceLayout>& resourceLayouts,
        std::vector<BatchShaderResource>& outResources
    )
    {
        for (size_t resourceIndex = 0; resourceIndex < outResources.size(); ++resourceIndex)
        {
            const ShaderResourceLayout& resourceLayout = resourceLayouts[resourceIndex];
            BatchShaderResource& outResource = outResources[resourceIndex];
            outResource.type = resourceLayout.type;
            outResource.buffer.resize(framesInFlight);
            outResource.descriptorSet.resize(framesInFlight);
            for (size_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
            {
                const DescriptorSetLayout& descriptorSetLayout = resourceLayout.descriptorSetLayout;
                const std::vector<DescriptorSetLayoutBinding>& descriptorSetLayoutBindings = descriptorSetLayout.getBindings();
                std::vector<DescriptorSetComponent> descriptorSetComponents(descriptorSetLayoutBindings.size());
                size_t useTextureIndex = 0;
                for (size_t i = 0; i < descriptorSetComponents.size(); ++i)
                {
                    const DescriptorSetLayoutBinding& binding = descriptorSetLayoutBindings[i];
                    const DescriptorType& descriptorType = binding.getType();
                    const std::vector<Texture*>& textures = resourceLayout.textures;
                    if (descriptorType == DescriptorType::DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    {
                        if (useTextureIndex >= textures.size())
                        {
                            Debug::log(
                                "@Batcher::createBatchShaderResources "
                                "Descriptor set layout binding was: DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER "
                                "at index: " + std::to_string(i) + " "
                                "but no texture provided(useTextureIndex = " + std::to_string(useTextureIndex) + ")",
                                Debug::MessageType::PLATYPUS_ERROR
                            );
                            PLATYPUS_ASSERT(false);
                            return;
                        }

                        descriptorSetComponents[i] = { descriptorType, textures[useTextureIndex] };
                        ++useTextureIndex;
                    }
                    else if (descriptorType == DescriptorType::DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                            descriptorType == DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER
                            )
                    {
                        const size_t bufferElementSize = resourceLayout.uniformBufferElementSize;
                        std::vector<char> bufferData(bufferElementSize * maxBatchLength);
                        memset(bufferData.data(), 0, bufferData.size());
                        Buffer* pUniformBuffer = new Buffer(
                            bufferData.data(),
                            bufferElementSize,
                            maxBatchLength,
                            BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                            true
                        );
                        outResource.buffer[frameIndex] = pUniformBuffer;

                        descriptorSetComponents[i] = { descriptorType, pUniformBuffer };
                    }
                }

                outResource.descriptorSet[frameIndex] = _descriptorPoolRef.createDescriptorSet(
                    descriptorSetLayout,
                    descriptorSetComponents
                );
            }
        }
        addToAllocatedShaderResources(batchID, outResources);
    }

    void Batcher::combineUsedDescriptorSets(
        size_t framesInFlight,
        const std::vector<std::vector<DescriptorSet>>& descriptorSetsToUse,
        std::vector<std::vector<DescriptorSet>>& outDescriptorSets
    )
    {
        for (size_t frame = 0; frame < framesInFlight; ++frame)
        {
            for (size_t descriptorSetIndex = 0; descriptorSetIndex < descriptorSetsToUse.size(); ++descriptorSetIndex)
            {
                outDescriptorSets[frame].push_back(descriptorSetsToUse[descriptorSetIndex][frame]);
            }
        }
    }

    Batch* Batcher::getBatch(ID_t batchID)
    {
        std::unordered_map<ID_t, size_t>::iterator it = _identifierBatchMapping.find(batchID);
        if (it == _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::getBatch "
                "Failed to find batch using identifier: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _batches[it->second];
    }

    Buffer* Batcher::getBatchBuffer(ID_t batchID, size_t resourceIndex, size_t frame)
    {
        std::unordered_map<ID_t, size_t>::const_iterator it = _batchShaderResourceMapping.find(batchID);
        if (it == _batchShaderResourceMapping.end())
        {
            Debug::log(
                "@Batcher::getBatchBuffer "
                "Failed to find batch shader resource using identifier: " + std::to_string(batchID) + " "
                "resourceIndex = " + std::to_string(resourceIndex),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        // TODO: Make safer?
        return _allocatedShaderResources[it->second][resourceIndex].buffer[frame];
    }

    bool Batcher::validateBatchDoesntExist(const char* callLocation, ID_t batchID) const
    {
        const std::string locationStr(callLocation);
        if (_identifierBatchMapping.find(batchID) != _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::" + locationStr +" "
                "Batch already exists for identifier: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    bool Batcher::validateDescriptorSetCounts(
        const char* callLocation,
        size_t framesInFlight,
        size_t commonDescriptorSetCount,
        size_t materialDescriptorSetCount
    ) const
    {
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != materialDescriptorSetCount))
        {
            std::string locationStr(callLocation);
            Debug::log(
                "@Batcher::" + locationStr + "(validateDescriptorSetCounts) "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    bool Batcher::validateDescriptorSetCounts(
        const char* callLocation,
        size_t framesInFlight,
        size_t commonDescriptorSetCount,
        size_t batchDescriptorSetCount,
        size_t materialDescriptorSetCount
    ) const
    {
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != batchDescriptorSetCount) ||
             (commonDescriptorSetCount != materialDescriptorSetCount) ||
             (materialDescriptorSetCount != batchDescriptorSetCount))
        {
            std::string locationStr(callLocation);
            Debug::log(
                "@Batcher::" + locationStr + "(validateDescriptorSetCounts) "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Batch descriptor set count: " + std::to_string(batchDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return false;
        }
        return true;
    }

    void Batcher::addToAllocatedShaderResources(
        ID_t batchID,
        std::vector<BatchShaderResource>& shaderResources
    )
    {
        if (_batchShaderResourceMapping.find(batchID) == _batchShaderResourceMapping.end())
        {
            _batchShaderResourceMapping[batchID] = _allocatedShaderResources.size();
            _allocatedShaderResources.push_back(shaderResources);
        }
        else
        {
            std::vector<BatchShaderResource>& existingResources = _allocatedShaderResources[_batchShaderResourceMapping[batchID]];
            existingResources.insert(existingResources.end(), shaderResources.begin(), shaderResources.end());
        }
    }

    // TODO: delete below after fixing static and skinned batches
    void Batcher::addToAllocatedShaderResources(
        ID_t batchID,
        const std::vector<Buffer*>& buffers,
        const std::vector<DescriptorSet> descriptorSets
    )
    {
        if (_batchShaderResourceMapping.find(batchID) == _batchShaderResourceMapping.end())
        {
            _batchShaderResourceMapping[batchID] = _allocatedShaderResources.size();
            _allocatedShaderResources.push_back({ { ShaderResourceType::ANY, buffers, descriptorSets } });
        }
        else
        {
            _allocatedShaderResources[_batchShaderResourceMapping[batchID]].push_back({ ShaderResourceType::ANY, buffers, descriptorSets });
        }
    }
}
