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
        size_t maxStaticBatchLength,
        size_t maxSkinnedBatchLength,
        size_t maxTerrainBatchLength,
        size_t maxSkinnedMeshJoints
    ) :
        _masterRendererRef(masterRenderer),
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


    // 1. Check that batch doesn't exist yet
    // 2. Check that "batch buffer" hasn't been created

    // NOTE: Could even create the pipelines dynamically here, so wouldn't require having those inside
    // Material anymore?
    ID_t Batcher::createStaticBatch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (_identifierBatchMapping.find(identifier) != _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::createStaticBatch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        if (_batchBufferMapping.find(identifier) != _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::createStaticBatch "
                "Instanced vertex buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        size_t descriptorSetCountPerFrame = _masterRendererRef.getScene3DDataDescriptorSets().size();
        if (pMaterial->getDescriptorSets().size() != descriptorSetCountPerFrame)
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Mismatch in descriptor set counts for frames! "
                "Common descriptor set count: " + std::to_string(descriptorSetCountPerFrame) + " "
                "Material descriptor set count: " + std::to_string(pMaterial->getDescriptorSets().size()),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        std::vector<std::vector<DescriptorSet>> allDescriptorSets(descriptorSetCountPerFrame);
        for (size_t i = 0; i < descriptorSetCountPerFrame; ++i)
        {
            allDescriptorSets[i] = {
                _masterRendererRef.getScene3DDataDescriptorSets()[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        // Create the instanced transforms buffer
        // NOTE: Should these be created separately for each frame in flight as well?
        std::vector<Matrix4f> transformsBuffer(_maxStaticBatchLength);
        const size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        std::vector<Buffer*> instancedBuffers(framesInFlight);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            instancedBuffers[i] = new Buffer(
                transformsBuffer.data(),
                sizeof(Matrix4f),
                transformsBuffer.size(),
                BufferUsageFlagBits::BUFFER_USAGE_VERTEX_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_STREAM,
                true
            );
        }
        _batchBufferMapping[identifier] = _allocatedBuffers.size();
        _allocatedBuffers.push_back(instancedBuffers);

        Batch* pBatch = new Batch{
            BatchType::STATIC_INSTANCED,
            pPipeline,
            allDescriptorSets,
            0, // dynamic uniform buffer element size
            { pMesh->getVertexBuffer() },
            { instancedBuffers },
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
        if (_identifierBatchMapping.find(identifier) != _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::createSkinnedBatch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        if (_batchBufferMapping.find(identifier) != _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::createSkinnedBatch "
                "Joint uniform buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        DescriptorPool& descriptorPool = _masterRendererRef.getDescriptorPool();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

        Pipeline* pPipeline = pMaterial->getSkinnedPipelineData()->pPipeline;

        // Create joint uniform buffer and descriptor sets
        size_t uniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * _maxSkinnedMeshJoints
        );
        std::vector<char> jointBufferData(uniformBufferElementSize * _maxSkinnedBatchLength);
        memset(jointBufferData.data(), 0, jointBufferData.size());

        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        std::vector<Buffer*> jointBuffers(framesInFlight);
        std::vector<DescriptorSet> jointDescriptorSets(framesInFlight);

        for (size_t i = 0; i < framesInFlight; ++i)
        {
            Buffer* pJointUniformBuffer = new Buffer(
                jointBufferData.data(),
                uniformBufferElementSize,
                _maxSkinnedBatchLength,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            jointBuffers[i] = pJointUniformBuffer;

            jointDescriptorSets[i] = descriptorPool.createDescriptorSet(
                &s_jointDescriptorSetLayout,
                {
                    { DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, pJointUniformBuffer }
                }
            );
        }

        _batchBufferMapping[identifier] = _allocatedBuffers.size();
        _allocatedBuffers.push_back(jointBuffers);

        _batchDescriptorSetMapping[identifier] = _descriptorSets.size();
        _descriptorSets.push_back(jointDescriptorSets);

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        const size_t commonDescriptorSetCount = _masterRendererRef.getScene3DDataDescriptorSets().size();
        const size_t materialDescriptorSetCount = pMaterial->getDescriptorSets().size();
        const size_t jointDataDescriptorSetCount = jointDescriptorSets.size();
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != materialDescriptorSetCount) ||
             (commonDescriptorSetCount != jointDataDescriptorSetCount) ||
             (materialDescriptorSetCount != jointDataDescriptorSetCount))
        {
            Debug::log(
                "@Batcher::createSkinnedBatch "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount) + " "
                "Joint descriptor set count: " + std::to_string(jointDataDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        std::vector<std::vector<DescriptorSet>> allDescriptorSets(framesInFlight);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            allDescriptorSets[i] = {
                _masterRendererRef.getScene3DDataDescriptorSets()[i],
                jointDescriptorSets[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        Batch* pBatch = new Batch{
            BatchType::SKINNED,
            pPipeline,
            allDescriptorSets,
            (uint32_t)uniformBufferElementSize, // dynamic uniform buffer element size
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
        if (_identifierBatchMapping.find(identifier) != _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::createTerrainBatch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        if (_batchBufferMapping.find(identifier) != _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::createTerrainBatch "
                "Uniform buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        AssetManager* pAssetManager = Application::get_instance()->getAssetManager();
        DescriptorPool& descriptorPool = _masterRendererRef.getDescriptorPool();

        TerrainMesh* pMesh = (TerrainMesh*)pAssetManager->getAsset(terrainMeshID, AssetType::ASSET_TYPE_TERRAIN_MESH);
        TerrainMaterial* pMaterial = (TerrainMaterial*)pAssetManager->getAsset(terrainMaterialID, AssetType::ASSET_TYPE_TERRAIN_MATERIAL);

        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

        // Create uniform buffers holding transformation matrix, tile size
        // and vertices. Also create descriptor sets for these
        // TODO: Some place to store the element size of the buffer
        //  -> might become issue when the elem size changes!
        size_t uniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) + sizeof(Vector2f)
        );
        std::vector<char> uniformBufferData(uniformBufferElementSize * _maxSkinnedBatchLength);
        memset(uniformBufferData.data(), 0, uniformBufferData.size());

        size_t framesInFlight = _masterRendererRef.getSwapchain().getMaxFramesInFlight();
        std::vector<Buffer*> uniformBuffers(framesInFlight);
        std::vector<DescriptorSet> terrainDataDescriptorSets(framesInFlight);

        for (size_t i = 0; i < framesInFlight; ++i)
        {
            Buffer* pJointUniformBuffer = new Buffer(
                uniformBufferData.data(),
                uniformBufferElementSize,
                _maxTerrainBatchLength,
                BufferUsageFlagBits::BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                BufferUpdateFrequency::BUFFER_UPDATE_FREQUENCY_DYNAMIC,
                true
            );
            uniformBuffers[i] = pJointUniformBuffer;

            terrainDataDescriptorSets[i] = descriptorPool.createDescriptorSet(
                &s_jointDescriptorSetLayout,
                {
                    { DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, pJointUniformBuffer }
                }
            );
        }

        _batchBufferMapping[identifier] = _allocatedBuffers.size();
        _allocatedBuffers.push_back(uniformBuffers);

        _batchDescriptorSetMapping[identifier] = _descriptorSets.size();
        _descriptorSets.push_back(terrainDataDescriptorSets);

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        const size_t commonDescriptorSetCount = _masterRendererRef.getScene3DDataDescriptorSets().size();
        const size_t materialDescriptorSetCount = pMaterial->getDescriptorSets().size();
        const size_t terrainDataDescriptorSetCount = terrainDataDescriptorSets.size();
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != materialDescriptorSetCount) ||
             (commonDescriptorSetCount != terrainDataDescriptorSetCount) ||
             (materialDescriptorSetCount != terrainDataDescriptorSetCount))
        {
            Debug::log(
                "@BatchPool::create_terrain_batch "
                "Mismatch in descriptor set counts for " + std::to_string(framesInFlight) + " frames in flight! "
                "Common descriptor set count: " + std::to_string(commonDescriptorSetCount) + " "
                "Material descriptor set count: " + std::to_string(materialDescriptorSetCount) + " "
                "Terrain data descriptor set count: " + std::to_string(terrainDataDescriptorSetCount),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        std::vector<std::vector<DescriptorSet>> allDescriptorSets(framesInFlight);
        for (size_t i = 0; i < framesInFlight; ++i)
        {
            allDescriptorSets[i] = {
                _masterRendererRef.getScene3DDataDescriptorSets()[i],
                terrainDataDescriptorSets[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        Batch* pBatch = new Batch{
            BatchType::SKINNED,
            pPipeline,
            allDescriptorSets,
            (uint32_t)uniformBufferElementSize, // dynamic uniform buffer element size
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

    void Batcher::addToStaticBatch(ID_t identifier, const Matrix4f& transformationMatrix, size_t currentFrame)
    {
        std::unordered_map<ID_t, size_t>::iterator batchIndexIt = _identifierBatchMapping.find(identifier);
        if (batchIndexIt == _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::addToStaticBatch "
                "Failed to find batch using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Batch* pBatch = _batches[batchIndexIt->second];

        std::unordered_map<ID_t, size_t>::const_iterator instancedBufferIndexIt = _batchBufferMapping.find(identifier);
        if (instancedBufferIndexIt == _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::addToStaticBatch "
                "Failed to find instanced buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
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

        _allocatedBuffers[instancedBufferIndexIt->second][currentFrame]->updateHost(
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
        std::unordered_map<ID_t, size_t>::iterator batchIndexIt = _identifierBatchMapping.find(identifier);
        if (batchIndexIt == _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::addToSkinnedBatch "
                "Failed to find batch using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Batch* pBatch = _batches[batchIndexIt->second];

        std::unordered_map<ID_t, size_t>::const_iterator jointBufferIndexIt = _batchBufferMapping.find(identifier);
        if (jointBufferIndexIt == _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::addToSkinnedBatch "
                "Failed to find joint uniform buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

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

        Buffer* pJointBuffer = _allocatedBuffers[jointBufferIndexIt->second][currentFrame];
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
        std::unordered_map<ID_t, size_t>::iterator batchIndexIt = _identifierBatchMapping.find(identifier);
        if (batchIndexIt == _identifierBatchMapping.end())
        {
            Debug::log(
                "@Batcher::addToTerrainBatch "
                "Failed to find batch using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Batch* pBatch = _batches[batchIndexIt->second];

        std::unordered_map<ID_t, size_t>::const_iterator terrainBufferIndexIt = _batchBufferMapping.find(identifier);
        if (terrainBufferIndexIt == _batchBufferMapping.end())
        {
            Debug::log(
                "@Batcher::addToTerrainBatch "
                "Failed to find terrain uniform buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

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

        Buffer* pTerrainBuffer = _allocatedBuffers[terrainBufferIndexIt->second][currentFrame];
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
}
