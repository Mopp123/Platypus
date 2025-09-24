#include "Batch.hpp"
#include "platypus/assets/TerrainMesh.hpp"
#include "platypus/assets/TerrainMaterial.hpp"
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
                    { { 5, ShaderDataType::Mat4, (int)_maxSkinnedMeshJoints } }
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
                        { 6, ShaderDataType::Mat4 },
                        { 7, ShaderDataType::Float2 },
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
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier) ||
            !validateBatchBufferDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier)
        )
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

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

        // TODO: Some better way to deal with this?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        const std::vector<DescriptorSet>& materialDescriptorSets = pMaterial->getDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            materialDescriptorSets.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                materialDescriptorSets
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::STATIC_INSTANCED,
            pPipeline,
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
            0 // instance count
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);
        return identifier;
    }

    ID_t Batcher::createSkinnedBatch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier) ||
            !validateBatchBufferDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier)
        )
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);
        Pipeline* pPipeline = pMaterial->getSkinnedPipelineData()->pPipeline;

        // Create joint uniform buffer and descriptor sets
        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        size_t dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * _maxSkinnedMeshJoints
        );
        std::vector<Buffer*> jointUniformBuffers(framesInFlight);
        std::vector<DescriptorSet> jointDescriptorSets(framesInFlight);
        createBatchShaderResources(
            identifier,
            dynamicUniformBufferElementSize,
            _maxSkinnedBatchLength,
            framesInFlight,
            s_jointDescriptorSetLayout,
            jointUniformBuffers,
            jointDescriptorSets
        );

        // TODO: Some better way to deal with this?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        const std::vector<DescriptorSet>& materialDescriptorSets = pMaterial->getDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            jointDescriptorSets.size(),
            materialDescriptorSets.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                jointDescriptorSets,
                materialDescriptorSets
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::SKINNED,
            pPipeline,
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
            0 // instance count
        };

        // TODO: Optimize? Maybe preallocate and don't push?
        _identifierBatchMapping[identifier] = _batches.size();
        _batches.push_back(pBatch);
        return identifier;
    }

    ID_t Batcher::createTerrainBatch(ID_t terrainMeshID, ID_t terrainMaterialID)
    {
        ID_t identifier = ID::hash(terrainMeshID, terrainMaterialID);
        if (!validateBatchDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier) ||
            !validateBatchBufferDoesntExist(PLATYPUS_CURRENT_FUNC_NAME, identifier)
        )
        {
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        TerrainMesh* pMesh = (TerrainMesh*)pAssetManager->getAsset(terrainMeshID, AssetType::ASSET_TYPE_TERRAIN_MESH);
        TerrainMaterial* pMaterial = (TerrainMaterial*)pAssetManager->getAsset(terrainMaterialID, AssetType::ASSET_TYPE_TERRAIN_MATERIAL);
        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

        // Create uniform buffers holding transformation matrix, tile size
        // and vertices. Also create descriptor sets for these
        // TODO: Some place to store the element size of the buffer
        //  -> might become issue when the elem size changes!
        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        size_t dynamicUniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) + sizeof(Vector2f)
        );
        std::vector<Buffer*> uniformBuffers(framesInFlight);
        std::vector<DescriptorSet> descriptorSets(framesInFlight);
        createBatchShaderResources(
            identifier,
            dynamicUniformBufferElementSize,
            _maxTerrainBatchLength,
            framesInFlight,
            s_terrainDescriptorSetLayout,
            uniformBuffers,
            descriptorSets
        );

        // TODO: Some better way to deal with these?
        const std::vector<DescriptorSet>& commonDescriptorSets = _masterRendererRef.getScene3DDataDescriptorSets();
        const std::vector<DescriptorSet>& materialDescriptorSets = pMaterial->getDescriptorSets();
        validateDescriptorSetCounts(
            PLATYPUS_CURRENT_FUNC_NAME,
            framesInFlight,
            commonDescriptorSets.size(),
            descriptorSets.size(),
            materialDescriptorSets.size()
        );

        std::vector<std::vector<DescriptorSet>> useDescriptorSets(framesInFlight);
        combineUsedDescriptorSets(
            framesInFlight,
            {
                commonDescriptorSets,
                descriptorSets,
                materialDescriptorSets
            },
            useDescriptorSets
        );

        Batch* pBatch = new Batch{
            BatchType::SKINNED,
            pPipeline,
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
            0 // instance count
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

        Buffer* pInstancedBuffer = getBatchBuffer(identifier, currentFrame);
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

        Buffer* pJointBuffer = getBatchBuffer(identifier, currentFrame);
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

        Buffer* pTerrainBuffer = getBatchBuffer(identifier, currentFrame);
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
        std::unordered_map<ID_t, size_t>::const_iterator it;
        for (it = _batchBufferMapping.begin(); it != _batchBufferMapping.end(); ++it)
        {
            // TODO: Make safer!
            const ID_t batchID = it->first;
            const Batch* pBatch = _batches[_identifierBatchMapping[batchID]];
            if (pBatch->instanceCount > 0 || pBatch->repeatCount > 0)
            {
                // TODO: Make safer!
                const size_t bufferIndex = _batchBufferMapping[batchID];
                Buffer* pBuffer = _allocatedBuffers[bufferIndex][currentFrame];
                pBuffer->updateDevice(
                    pBuffer->accessData(),
                    pBuffer->getTotalSize(),
                    0
                );
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

        // Free descriptor sets
        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();
        for (std::vector<DescriptorSet>& descriptorSets : _descriptorSets)
            descriptorPool.freeDescriptorSets(descriptorSets);

        _descriptorSets.clear();
        _batchDescriptorSetMapping.clear();

        // Free buffers
        for (std::vector<Buffer*>& buffers : _allocatedBuffers)
        {
            for (Buffer* pBuffer : buffers)
                delete pBuffer;

            buffers.clear();
        }
        _allocatedBuffers.clear();
        _batchBufferMapping.clear();
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
        _batchBufferMapping[batchID] = _allocatedBuffers.size();
        _allocatedBuffers.push_back(outBuffers);
    }

    void Batcher::createBatchShaderResources(
        ID_t batchID,
        size_t bufferElementSize,
        size_t maxBatchLength,
        size_t framesInFlight,
        const DescriptorSetLayout& descriptorSetLayout,
        std::vector<Buffer*>& outUniformBuffers,
        std::vector<DescriptorSet>& outDescriptorSets
    )
    {
        std::vector<char> bufferData(bufferElementSize * maxBatchLength);
        memset(bufferData.data(), 0, bufferData.size());

        for (size_t i = 0; i < framesInFlight; ++i)
        {
            Buffer* pUniformBuffer = new Buffer(
                bufferData.data(),
                bufferElementSize,
                maxBatchLength,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            outUniformBuffers[i] = pUniformBuffer;

            outDescriptorSets[i] = _descriptorPoolRef.createDescriptorSet(
                &descriptorSetLayout,
                {
                    { DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, pUniformBuffer }
                }
            );
        }

        _batchBufferMapping[batchID] = _allocatedBuffers.size();
        _allocatedBuffers.push_back(outUniformBuffers);

        _batchDescriptorSetMapping[batchID] = _descriptorSets.size();
        _descriptorSets.push_back(outDescriptorSets);
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

    Buffer* Batcher::getBatchBuffer(ID_t batchID, size_t frame)
    {
        std::unordered_map<ID_t, size_t>::const_iterator it = _batchBufferMapping.find(batchID);
        if (it == _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::getBatchBuffer "
                "Failed to find batch buffer using identifier: " + std::to_string(batchID),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return nullptr;
        }
        return _allocatedBuffers[it->second][frame];
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

    bool Batcher::validateBatchBufferDoesntExist(const char* callLocation, ID_t batchID) const
    {
        const std::string locationStr(callLocation);
        if (_batchBufferMapping.find(batchID) != _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::addToStaticBatch "
                "Batch buffer already exists for identifier: " + std::to_string(batchID),
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
}
