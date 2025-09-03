#include "Batch.hpp"
#include "platypus/core/Application.h"
#include "platypus/core/Debug.h"
#include "platypus/graphics/Device.hpp"


namespace platypus
{
    std::vector<Batch*> BatchPool::s_batches;
    std::unordered_map<ID_t, size_t> BatchPool::s_identifierBatchMapping;
    size_t BatchPool::s_maxStaticBatchLength = 10000; // TODO: Make this configurable
    size_t BatchPool::s_maxSkinnedBatchLength = 1024; // TODO: Make this configurable
    size_t BatchPool::s_maxJoints = 50;

    DescriptorSetLayout BatchPool::s_jointDescriptorSetLayout;

    std::vector<std::vector<Buffer*>> BatchPool::s_allocatedBuffers;
    std::vector<std::vector<DescriptorSet>> BatchPool::s_descriptorSets;
    std::unordered_map<ID_t, size_t> BatchPool::s_batchBufferMapping;
    std::unordered_map<ID_t, size_t> BatchPool::s_batchDescriptorSetMapping;


    void BatchPool::init()
    {
        s_jointDescriptorSetLayout = DescriptorSetLayout(
            {
                {
                    0,
                    1,
                    DescriptorType::DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, // NOTE: Should probably be dynamix uniform buffer...
                    ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
                    { { 5, ShaderDataType::Mat4, (int)s_maxJoints } }
                }
            }
        );
    }

    void BatchPool::destroy()
    {
        s_jointDescriptorSetLayout.destroy();
    }

    ID_t BatchPool::get_batch_id(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);

        std::unordered_map<ID_t, size_t>::const_iterator indexIt = s_identifierBatchMapping.find(identifier);
        if (indexIt != s_identifierBatchMapping.end())
            return indexIt->first;

        return NULL_ID;
    }

    // NOTE: Could even create the pipelines dynamically here, so wouldn't require having those inside
    // Material anymore?
    ID_t BatchPool::create_static_batch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (s_identifierBatchMapping.find(identifier) != s_identifierBatchMapping.end())
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        if (s_batchBufferMapping.find(identifier) != s_batchBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::create_static_batch "
                "Instanced vertex buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

        BatchType type = BatchType::STATIC_INSTANCED;
        Pipeline* pPipeline = pMaterial->getPipelineData()->pPipeline;

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        size_t descriptorSetCountPerFrame = pMasterRenderer->getScene3DDataDescriptorSets().size();
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
                pMasterRenderer->getScene3DDataDescriptorSets()[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        // Create the instanced transforms buffer
        // NOTE: Should these be created separately for each frame in flight as well?
        std::vector<Matrix4f> transformsBuffer(s_maxStaticBatchLength);
        const size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
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
        s_batchBufferMapping[identifier] = s_allocatedBuffers.size();
        s_allocatedBuffers.push_back(instancedBuffers);

        Batch* pBatch = new Batch{
            type,
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
        s_identifierBatchMapping[identifier] = s_batches.size();
        s_batches.push_back(pBatch);
        return identifier;
    }

    ID_t BatchPool::create_skinned_batch(ID_t meshID, ID_t materialID)
    {
        ID_t identifier = ID::hash(meshID, materialID);
        if (s_identifierBatchMapping.find(identifier) != s_identifierBatchMapping.end())
        {
            Debug::log(
                "@BatchPool::create_skinned_batch "
                "Batch already exists for identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }
        if (s_batchBufferMapping.find(identifier) != s_batchBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::create_skinned_batch "
                "Joint uniform buffer has already been created for batch identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return NULL_ID;
        }

        Application* pApp = Application::get_instance();
        AssetManager* pAssetManager = pApp->getAssetManager();
        MasterRenderer* pMasterRenderer = pApp->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();

        Mesh* pMesh = (Mesh*)pAssetManager->getAsset(meshID, AssetType::ASSET_TYPE_MESH);
        Material* pMaterial = (Material*)pAssetManager->getAsset(materialID, AssetType::ASSET_TYPE_MATERIAL);

        BatchType type = BatchType::SKINNED;
        Pipeline* pPipeline = pMaterial->getSkinnedPipelineData()->pPipeline;

        // Create joint uniform buffer and descriptor sets
        size_t uniformBufferElementSize = get_dynamic_uniform_buffer_element_size(
            sizeof(Matrix4f) * s_maxJoints
        );
        std::vector<char> jointBufferData(uniformBufferElementSize * s_maxSkinnedBatchLength);
        memset(jointBufferData.data(), 0, jointBufferData.size());

        size_t framesInFlight = pMasterRenderer->getSwapchain().getMaxFramesInFlight();
        std::vector<Buffer*> jointBuffers(framesInFlight);
        std::vector<DescriptorSet> jointDescriptorSets(framesInFlight);

        for (size_t i = 0; i < framesInFlight; ++i)
        {
            Buffer* pJointUniformBuffer = new Buffer(
                jointBufferData.data(),
                uniformBufferElementSize,
                s_maxSkinnedBatchLength,
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

        s_batchBufferMapping[identifier] = s_allocatedBuffers.size();
        s_allocatedBuffers.push_back(jointBuffers);

        s_batchDescriptorSetMapping[identifier] = s_descriptorSets.size();
        s_descriptorSets.push_back(jointDescriptorSets);

        // NOTE: DANGER! Pretty fucked up...
        // TODO: Some better way to deal with these?
        const size_t commonDescriptorSetCount = pMasterRenderer->getScene3DDataDescriptorSets().size();
        const size_t materialDescriptorSetCount = pMaterial->getDescriptorSets().size();
        const size_t jointDataDescriptorSetCount = jointDescriptorSets.size();
        if ((commonDescriptorSetCount != framesInFlight) ||
             (commonDescriptorSetCount != materialDescriptorSetCount) ||
             (commonDescriptorSetCount != jointDataDescriptorSetCount) ||
             (materialDescriptorSetCount != jointDataDescriptorSetCount))
        {
            Debug::log(
                "@BatchPool::create_skinned_batch "
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
                pMasterRenderer->getScene3DDataDescriptorSets()[i],
                jointDescriptorSets[i],
                pMaterial->getDescriptorSets()[i]
            };
        }

        Batch* pBatch = new Batch{
            type,
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
        s_identifierBatchMapping[identifier] = s_batches.size();
        s_batches.push_back(pBatch);
        return identifier;
    }

    void BatchPool::add_to_static_batch(ID_t identifier, const Matrix4f& transformationMatrix, size_t currentFrame)
    {
        std::unordered_map<ID_t, size_t>::iterator batchIndexIt = s_identifierBatchMapping.find(identifier);
        if (batchIndexIt == s_identifierBatchMapping.end())
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Failed to find batch using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Batch* pBatch = s_batches[batchIndexIt->second];

        std::unordered_map<ID_t, size_t>::const_iterator instancedBufferIndexIt = s_batchBufferMapping.find(identifier);
        if (instancedBufferIndexIt == s_batchBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Failed to find instanced buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->instanceCount >= s_maxStaticBatchLength)
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Batch is full. Maximum static batch length is " + std::to_string(s_maxStaticBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        s_allocatedBuffers[instancedBufferIndexIt->second][currentFrame]->updateHost(
            (void*)(&transformationMatrix),
            sizeof(Matrix4f),
            sizeof(Matrix4f) * pBatch->instanceCount
        );
        ++pBatch->instanceCount;
        pBatch->repeatCount = 1;
    }

    void BatchPool::add_to_skinned_batch(
        ID_t identifier,
        void* pJointData,
        size_t jointDataSize,
        size_t currentFrame
    )
    {
        std::unordered_map<ID_t, size_t>::iterator batchIndexIt = s_identifierBatchMapping.find(identifier);
        if (batchIndexIt == s_identifierBatchMapping.end())
        {
            Debug::log(
                "@BatchPool::add_to_skinned_batch "
                "Failed to find batch using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }
        Batch* pBatch = s_batches[batchIndexIt->second];

        std::unordered_map<ID_t, size_t>::const_iterator jointBufferIndexIt = s_batchBufferMapping.find(identifier);
        if (jointBufferIndexIt == s_batchBufferMapping.end())
        {
            Debug::log(
                "@BatchPool::add_to_skinned_batch "
                "Failed to find joint uniform buffer using identifier: " + std::to_string(identifier),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        // TODO: Create new batch if this one is full!
        //  -> Need to have some kind of mapping where multiple batches can exist for the same identifier
        if (pBatch->repeatCount >= s_maxSkinnedBatchLength)
        {
            Debug::log(
                "@BatchPool::add_to_static_batch "
                "Batch is full. Maximum skinned batch length is " + std::to_string(s_maxSkinnedBatchLength),
                Debug::MessageType::PLATYPUS_ERROR
            );
            PLATYPUS_ASSERT(false);
            return;
        }

        Buffer* pJointBuffer = s_allocatedBuffers[jointBufferIndexIt->second][currentFrame];
        uint32_t jointBufferOffset = pBatch->repeatCount * pJointBuffer->getDataElemSize();
        pJointBuffer->updateHost(
            pJointData,
            jointDataSize,
            jointBufferOffset
        );
        pBatch->instanceCount = 1;
        ++pBatch->repeatCount;
    }

    void BatchPool::update_device_side_buffers(size_t currentFrame)
    {
        std::unordered_map<ID_t, size_t>::const_iterator it;
        for (it = s_batchBufferMapping.begin(); it != s_batchBufferMapping.end(); ++it)
        {
            // TODO: Make safer!
            const ID_t batchID = it->first;
            const Batch* pBatch = s_batches[s_identifierBatchMapping[batchID]];
            if (pBatch->instanceCount > 0 || pBatch->repeatCount > 0)
            {
                // TODO: Make safer!
                const size_t bufferIndex = s_batchBufferMapping[batchID];
                Buffer* pBuffer = s_allocatedBuffers[bufferIndex][currentFrame];
                pBuffer->updateDevice(
                    pBuffer->accessData(),
                    pBuffer->getTotalSize(),
                    0
                );
            }
        }
    }

    void BatchPool::reset_for_next_frame()
    {
        for (Batch* pBatch : s_batches)
        {
            pBatch->repeatCount = 0;
            pBatch->instanceCount = 0;
        }
    }

    void BatchPool::free_batches()
    {
        // Free batches themselves
        for (Batch* pBatch : s_batches)
            delete pBatch;

        s_batches.clear();
        s_identifierBatchMapping.clear();

        // Free descriptor sets
        MasterRenderer* pMasterRenderer = Application::get_instance()->getMasterRenderer();
        DescriptorPool& descriptorPool = pMasterRenderer->getDescriptorPool();
        for (std::vector<DescriptorSet>& descriptorSets : s_descriptorSets)
            descriptorPool.freeDescriptorSets(descriptorSets);

        s_descriptorSets.clear();
        s_batchDescriptorSetMapping.clear();

        // Free buffers
        for (std::vector<Buffer*>& buffers : s_allocatedBuffers)
        {
            for (Buffer* pBuffer : buffers)
                delete pBuffer;

            buffers.clear();
        }
        s_allocatedBuffers.clear();
        s_batchBufferMapping.clear();
    }

    const std::vector<Batch*>& BatchPool::get_batches()
    {
        return s_batches;
    }

    const DescriptorSetLayout& BatchPool::get_joint_descriptor_set_layout()
    {
        return s_jointDescriptorSetLayout;
    }
}
